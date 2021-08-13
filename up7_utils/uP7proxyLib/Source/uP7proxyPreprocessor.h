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
#ifndef UP7_PROXY_PREPROCESSOR_H
#define UP7_PROXY_PREPROCESSOR_H

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct stPreProcessorFile
{
    stPreProcessorFile(CWString *i_pFileName, uint32_t i_uSessionId, uint8_t i_uSessionCrc7)
    {
        pFileName    = i_pFileName;
        uSessionId   = i_uSessionId;
        uSessionCrc7 = i_uSessionCrc7;
        pData        = NULL;
        szData       = 0;
    }

    ~stPreProcessorFile()
    {
        if (pFileName) { delete pFileName; pFileName = NULL;}
        if (pData)     { free(pData); pData = NULL;         }
        uSessionId = 0;
        szData     = 0;
    }

    uint32_t  uSessionId;
    uint8_t   uSessionCrc7;
    CWString *pFileName;
    uint8_t  *pData;
    size_t    szData;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CPreProcessorFilesTree:
    public CRBTree<stPreProcessorFile*, uint32_t>
{
public:
    virtual ~CPreProcessorFilesTree() { Clear(); }
protected:
    virtual tBOOL Data_Release(stPreProcessorFile* i_pData)         { delete i_pData; return TRUE;          }
    tBOOL Is_Key_Less(uint32_t i_uKey, stPreProcessorFile *i_pData) { return i_uKey < i_pData->uSessionId;  }
    tBOOL Is_Qual(uint32_t i_uKey, stPreProcessorFile *i_pData)     { return i_uKey == i_pData->uSessionId; }
};

#endif //UP7_PROXY_PREPROCESSOR_H