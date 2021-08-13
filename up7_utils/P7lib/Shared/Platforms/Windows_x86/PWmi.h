////////////////////////////////////////////////////////////////////////////////
//                                                                             /
// 2012-2021 (c) Baical                                                        /
//                                                                             /
// This library is free software; you can redistribute it and/or               /
// modify it under the terms of the GNU Lesser General Public                  /
// License as published by the Free Software Foundation; either                /
// version 3.0 of the License, or (at your option) any later version.          /
//                                                                             /
// This library is distributed in the hope that it will be useful,             /
// but WITHOUT ANY WARRANTY; without even the implied warranty of              /
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU           /
// Lesser General Public License for more details.                             /
//                                                                             /
// You should have received a copy of the GNU Lesser General Public            /
// License along with this library.                                            /
//                                                                             /
////////////////////////////////////////////////////////////////////////////////
#ifndef PWMI_H
#define PWMI_H

#include "Wbemcli.h"
#include <wbemidl.h>
#include <comutil.h>

/////////////////////////////////////////////////////////////////////////////////
//Dependencies !!!!
/////////////////////////////////////////////////////////////////////////////////
//#pragma comment(lib, "wbemuuid.lib")
//#pragma comment(lib, "comsuppw.lib")

class CWMI
{
    tBOOL                 m_bCom;
    IWbemServices        *m_pWbemServices;
    IEnumWbemClassObject *m_pEnumerator;
    IWbemClassObject     *m_pObject;
    tBOOL                 m_bIninitialized;
public:
    ////////////////////////////////////////////////////////////////////////////
    //CWMI
    CWMI(const wchar_t *i_pQuery, const wchar_t *i_pNameSpace = L"\\\\.\\root\\cimv2")
        : m_bCom(FALSE)
        , m_pWbemServices(NULL)
        , m_pEnumerator(NULL)
        , m_pObject(NULL)
        , m_bIninitialized(TRUE)
    {
        HRESULT       l_hResult       = CoInitialize(NULL);
        //HRESULT       l_hResult       = CoInitializeEx(0, COINIT_MULTITHREADED);
        IWbemLocator *l_pIWbemLocator = NULL;

        if (S_OK == l_hResult)
        {
            m_bCom = TRUE;
        }
        else if (S_FALSE == l_hResult)
        {
            m_bCom = FALSE;
        }
        else
        {
            m_bIninitialized = FALSE;
            goto l_lblExit;
        }

        l_hResult =  CoInitializeSecurity(NULL,                        // Security descriptor    
                                          -1,                          // COM negotiates authentication service
                                          NULL,                        // Authentication services
                                          NULL,                        // Reserved
                                          RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication level for proxies
                                          RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation level for proxies
                                          NULL,                        // Authentication info
                                          EOAC_NONE,                   // Additional capabilities of the client or server
                                          NULL                         // Reserved
                                         );                       

        //if (FAILED(l_hResult))
        //{
        //    goto l_lblExit;
        //}

        l_hResult = CoCreateInstance(CLSID_WbemAdministrativeLocator,
                                     NULL ,
                                     CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER , 
                                     IID_IUnknown ,
                                     ( void ** ) &l_pIWbemLocator
                                    );


        if (SUCCEEDED(l_hResult))
        {
            l_hResult = l_pIWbemLocator->ConnectServer(bstr_t(i_pNameSpace), // Namespace
                                                       NULL,                 // Userid
                                                       NULL,                 // PW
                                                       NULL,                 // Locale
                                                       0,                    // flags
                                                       NULL,                 // Authority
                                                       NULL,                 // Context
                                                       &m_pWbemServices
                                                      );

            l_pIWbemLocator->Release(); // Free memory resources.
            l_pIWbemLocator = NULL;
        }

        if (FAILED(l_hResult))
        {
            goto l_lblExit;
        }

        l_hResult = CoSetProxyBlanket(m_pWbemServices,
                                      RPC_C_AUTHN_WINNT,
                                      RPC_C_AUTHZ_NONE,
                                      NULL,
                                      RPC_C_AUTHN_LEVEL_CALL,
                                      RPC_C_IMP_LEVEL_IMPERSONATE,
                                      NULL,
                                      EOAC_NONE
                                     );

        if (FAILED(l_hResult))
        {
            goto l_lblExit;
        }


        l_hResult = m_pWbemServices->ExecQuery(bstr_t("WQL"),
                                               bstr_t(i_pQuery),
                                               WBEM_FLAG_BIDIRECTIONAL | WBEM_FLAG_RETURN_IMMEDIATELY,
                                               NULL,
                                               &m_pEnumerator
                                              );

        if (FAILED(l_hResult))
        {
            goto l_lblExit;
        }


    l_lblExit:
        if (FAILED(l_hResult))
        {
            m_bIninitialized = FALSE;
        }
        else
        {
            m_bIninitialized = Reset();
        }
    }//CWMI

    ////////////////////////////////////////////////////////////////////////////
    //~CWMI
    ~CWMI()
    {
        if (m_pObject)
        {
            m_pObject->Release();
            m_pObject = NULL;
        }

        if (m_pEnumerator)
        {
            m_pEnumerator->Release();
            m_pEnumerator = NULL;
        }

        if (m_pWbemServices)
        {
            m_pWbemServices->Release();
            m_pWbemServices = NULL;
        }

        if (m_bCom)
        {
            CoUninitialize();
        }
    }//~CWMI


    ////////////////////////////////////////////////////////////////////////////
    //Initialized
    tBOOL Initialized()
    {
        return m_bIninitialized;
    }//Initialized


    ////////////////////////////////////////////////////////////////////////////
    //Reset
    tBOOL Reset()
    {
        if (!m_bIninitialized)
        {
            return FALSE;
        }

        if (m_pObject)
        {
            m_pObject->Release();
            m_pObject = NULL;
        }

        m_pEnumerator->Reset();

        return Next();
    }//Reset


    ////////////////////////////////////////////////////////////////////////////
    //Next
    tBOOL Next()
    {
        if (!m_bIninitialized)
        {
            return FALSE;
        }

        if (m_pObject)
        {
            m_pObject->Release();
            m_pObject = NULL;
        }

        tBOOL   l_bReturn = FALSE;
        ULONG   l_uReturn = 0;
        HRESULT l_hResult = m_pEnumerator->Next(WBEM_INFINITE, 
                                                1,
                                                &m_pObject, 
                                                &l_uReturn
                                               );

        if (    (l_uReturn)
             && (SUCCEEDED(l_hResult))
           )
        {
            return TRUE;
        }

        return FALSE;
    }//Next


    ////////////////////////////////////////////////////////////////////////////
    //Get_Text
    tBOOL Get_Text(const wchar_t *i_pName, CWString &o_rValue)
    {
        if (    (!m_bIninitialized)
             || (!i_pName)
             || (!m_pObject)
           )
        {
            return FALSE;
        }

        tBOOL l_bReturn = FALSE;
        VARIANT l_cProperty; 
        memset(&l_cProperty, 0, sizeof(l_cProperty));

        l_cProperty.vt = VT_EMPTY;

        HRESULT l_hResult = m_pObject->Get(i_pName, 0, &l_cProperty, 0, 0);
        if (VT_BSTR == l_cProperty.vt)
        {
            l_bReturn = TRUE;
            o_rValue.Set(l_cProperty.bstrVal);

        }

        VariantClear(&l_cProperty);
        
        return l_bReturn;
    }//Get_Text
};



#endif //PWMI_H