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
#include "uP7preCommon.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CSessionFile::CSessionFile(const tXCHAR *i_pName)
    : m_szData(0)
    , m_szDataMax(0)
    , m_pData(nullptr) 
    , m_pHdr(nullptr)
    , m_eError(eErrorNo)
{
    memset(m_pHash, 0, sizeof(m_pHash));

    if (CFSYS::File_Exists(i_pName))
    {
       CPFile  l_cuP7Binfile;
       if (l_cuP7Binfile.Open(i_pName, IFile::EOPEN | IFile::EACCESS_READ | IFile::ESHARE_READ))
        {
            m_szData    = (size_t)l_cuP7Binfile.Get_Size();
            m_szDataMax = m_szData;
            if (m_szData)
            {
                m_pData = (tUINT8*)malloc(m_szData);
                if (m_pData)
                {
                    if (m_szData != l_cuP7Binfile.Read(m_pData, m_szData))
                    {
                        m_eError = eErrorFileRead;
                        m_szData = 0;
                    }
                    else
                    {
                        m_pHdr = (stuP7SessionFileHeader *)m_pData;
                    }
                }
                else
                {
                    OSPRINT(TM("WARNING: Can't allocate memory %zu\n"), m_szData);
                    m_eError = eErrorMemAlloc;
                    m_szData = 0;
                }
            }


            l_cuP7Binfile.Close(FALSE);
        }
        else
        {
            m_eError = eErrorFileOpen;
        }
    }
    else
    {
        m_eError = eErrorFileOpen;
    }

    UpdateHash();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CSessionFile::CSessionFile(CBList<CpreFile*> *i_pFiles, tUINT32  i_uSession, uint64_t i_qwEpochTime)
    : m_szData(0)
    , m_szDataMax(1024u*1024u)
    , m_pData((tUINT8*)malloc(m_szDataMax)) 
    , m_pHdr((stuP7SessionFileHeader *)m_pData)
    , m_eError(eErrorNo)
{
    memset(m_pHash, 0, sizeof(m_pHash));

    if (!m_pData)
    {
        OSPRINT(TM("ERROR: Can't allocate memory %zu bytes\n"), m_szData);
        m_eError = eErrorMemAlloc;
        return;
    }


    m_pHdr->qwTimeStamp = i_qwEpochTime;
    m_pHdr->uSessionId  = i_uSession;
    m_pHdr->uVersion    = SESSION_ID_FILE_VER;

    m_szData = sizeof(stuP7SessionFileHeader);

    //generate IDs & serialize binary data
    pAList_Cell l_pFileEl = NULL;
    while (    ((l_pFileEl = i_pFiles->Get_Next(l_pFileEl)))
            && (eErrorNo == m_eError)
          )
    {
        CpreFile *l_pFile = i_pFiles->Get_Data(l_pFileEl);
        if (l_pFile)
        {
            CBList<CFuncRoot*> &l_rFunctions = l_pFile->GetFunctions();

            pAList_Cell l_pFuncEl = NULL;
            while ((l_pFuncEl = l_rFunctions.Get_Next(l_pFuncEl)))
            {
                CFuncRoot *l_pFunc = l_rFunctions.Get_Data(l_pFuncEl);

                if (eErrorNo == m_eError)
                {
                    size_t l_szBuf = 0;
                    void  *l_pBuf  = l_pFunc->GetBuffer(l_szBuf);

                    if ((l_szBuf + m_szData) >= m_szDataMax)
                    {
                        size_t  l_szTemp = m_szDataMax * 2;
                        tUINT8 *l_pTemp  = (tUINT8*)realloc(m_pData, l_szTemp);
                        if (l_pTemp)
                        {
                            m_pData     = l_pTemp;
                            m_szDataMax = l_szTemp;
                        }
                        else
                        {
                            OSPRINT(TM("ERROR: Can't allocate memory %zu bytes\n"), l_szTemp);
                            m_eError = eErrorMemAlloc;
                        }
                    }

                    if (eErrorNo == m_eError)
                    {
                        memcpy(m_pData + m_szData, l_pBuf, l_szBuf);
                        m_szData += l_szBuf;
                    }
                }
            }
        }
    }

    UpdateHash();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CSessionFile::~CSessionFile()
{
    if (m_pData)
    {
        free(m_pData);
        m_pData = nullptr;
    }

    m_szData = 0;
    m_pHdr   = nullptr;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
eErrorCodes CSessionFile::SetSession(uint32_t i_uSession, uint8_t i_uSessionCrc7, uint64_t i_qwEpochTime)
{
    if (eErrorNo != m_eError)
    {
        return m_eError;
    }

    m_pHdr->qwTimeStamp = i_qwEpochTime;
    m_pHdr->uSessionId  = i_uSession;
    m_pHdr->uCrc7       = i_uSessionCrc7;
    m_pHdr->uFlags      = 0;
    

    return m_eError;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
eErrorCodes CSessionFile::Save(const tXCHAR *i_pName)
{
    if (eErrorNo != m_eError)
    {
        return m_eError;
    }

    CPFile l_cuP7Binfile;

    if (l_cuP7Binfile.Open(i_pName, IFile::ECREATE | IFile::ESHARE_READ | IFile::EACCESS_WRITE))
    {
        if (m_szData != l_cuP7Binfile.Write(m_pData, m_szData, FALSE))
        {
            OSPRINT(TM("ERROR: Can't write file {%s}\n"), i_pName);
            return eErrorFileWrite;
        }

        l_cuP7Binfile.Close(TRUE);
    }
    else
    {
        OSPRINT(TM("ERROR: Can't write file {%s}\n"), i_pName);
        return eErrorFileWrite;
    }

    return m_eError;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CSessionFile::UpdateHash()
{
    if (    (m_eError == eErrorNo)
         && (m_szData > sizeof(stuP7SessionFileHeader))
       )
    {
        CKeccak l_cHash;
        l_cHash.UpdateB(m_pData + sizeof(stuP7SessionFileHeader), m_szData - sizeof(stuP7SessionFileHeader));
        l_cHash.Get_HashB(m_pHash, sizeof(m_pHash));
    }
}
