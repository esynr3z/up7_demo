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
#ifndef PSTACKTRACE_H
#define PSTACKTRACE_H

#include <DbgHelp.h>

#if !defined(STACK_FUNCTION_NAME_LEN)
    #define STACK_FUNCTION_NAME_LEN   (96)
#endif

#if !defined(STACK_TRACE_LENGTH)
    #define STACK_TRACE_LENGTH        (16)
#endif

class CStackTrace
{
public:
    struct sStack
    {
        tXCHAR  pName[STACK_FUNCTION_NAME_LEN];
        tUINT64 pAddr;
        tUINT32 dwLine;
    };

private:
    CWString      m_cPath;
    sStack       *m_pStack;
    void        **m_pAddr;
    SYMBOL_INFOW *m_pSymbol;
    tBOOL         m_bInitialized;

public:
    CStackTrace()
        : m_pStack(0)
        , m_pAddr(0)
        , m_pSymbol(0)
        , m_bInitialized(TRUE)
    {
        m_pStack  = (sStack*)malloc(sizeof(sStack) * STACK_TRACE_LENGTH);
        m_pAddr   = (void**)malloc(sizeof(void*) * STACK_TRACE_LENGTH);
        m_pSymbol = (SYMBOL_INFOW*)calloc(   sizeof(SYMBOL_INFOW)
                                           + (STACK_FUNCTION_NAME_LEN * sizeof(wchar_t)),
                                          1
                                         );


        if (m_pSymbol)
        {
            m_pSymbol->MaxNameLen   = 255;
            m_pSymbol->SizeOfStruct = sizeof( SYMBOL_INFOW );
        }

        if (    (NULL == m_pStack)
             || (NULL == m_pAddr)
             || (NULL == m_pSymbol)
           )
        {
            m_bInitialized = FALSE;
        }


        if (m_bInitialized)
        {
            m_cPath.Realloc(2048);
            m_bInitialized = CProc::Get_Process_Path(m_cPath.Get(), m_cPath.Max_Length());
        }
    }

    ~CStackTrace()
    {
        if (m_pStack)
        {
            free(m_pStack);
            m_pStack = NULL;
        }

        if (m_pAddr)
        {
            free(m_pAddr);
            m_pAddr = NULL;
        }

        if (m_pSymbol)
        {
            free(m_pSymbol);
            m_pSymbol = NULL;
        }
    }

    Load DBG functions dynamically !!!!!
    http://vcpptips.wordpress.com/tag/symfromaddr/

    ////////////////////////////////////////////////////////////////////////////
    //Get_Stack
    const sStack *Get_Stack(tUINT32 *o_pCount, tUINT32 i_dwOffset)
    {
        tUINT16          l_wCount   = 0;
        HANDLE           l_hProcess = GetCurrentProcess();
        IMAGEHLP_LINEW64 l_sLine    = {0};
        DWORD            l_dwDisp   = 0;

        if (NULL == o_pCount)
        {
            return NULL;
        }

        l_sLine.SizeOfStruct = sizeof(IMAGEHLP_LINEW64);

        ::SymSetOptions(   SYMOPT_DEFERRED_LOADS
                         | SYMOPT_INCLUDE_32BIT_MODULES
                         | SYMOPT_UNDNAME 
                         | SYMOPT_LOAD_LINES
                       );

        if (SymInitializeW( l_hProcess, m_cPath.Get(), TRUE ))
        {
            l_wCount = CaptureStackBackTrace(i_dwOffset, 
                                             STACK_TRACE_LENGTH, 
                                             m_pAddr, 
                                             NULL
                                            );

            if (l_wCount)
            {
                for(tUINT16 l_wI = 0; l_wI < l_wCount; l_wI++ )
                {
                    if (SymFromAddrW(l_hProcess, (DWORD64)(m_pAddr[l_wI]), 0, m_pSymbol))
                    {
                        m_pStack[l_wI].pAddr = m_pSymbol->Address;
                        wcsncpy_s(m_pStack[l_wI].pName, 
                                  STACK_FUNCTION_NAME_LEN, 
                                  m_pSymbol->Name, 
                                  STACK_FUNCTION_NAME_LEN
                                 );
                    }
                    else
                    {
                        m_pStack[l_wI].pAddr  = 0ULL;
                        wcsncpy_s(m_pStack[l_wI].pName, 
                                  STACK_FUNCTION_NAME_LEN, 
                                  L"Unknown", 
                                  STACK_FUNCTION_NAME_LEN
                                 );
                    }

                    if (SymGetLineFromAddrW64(l_hProcess, 
                                              (DWORD64)(m_pAddr[l_wI]),
                                              &l_dwDisp,
                                              &l_sLine
                                             )
                       )
                    {
                        m_pStack[l_wI].dwLine  = l_sLine.LineNumber;
                    }
                    else
                    {
                        m_pStack[l_wI].dwLine  = 0;
                    }

                    m_pStack[l_wI].pName[STACK_FUNCTION_NAME_LEN - 1] = 0;
                }
            }

            SymCleanup( l_hProcess );
        }

        *o_pCount = l_wCount;

        return (l_wCount) ? m_pStack : NULL;
    }//Get_Stack
};


#endif //PSTACKTRACE_H