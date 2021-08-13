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
#ifndef UP7_FILE_H
#define UP7_FILE_H

class CpreFile
{
public:
    enum eModification
    {
        eReadOnly,
        eNotModified,
        eModified
    };

private:
    struct stBlock
    {
        tBOOL bFunction;
        int   iBrakets;
        char *pName;

        stBlock(tBOOL i_bFunction, const char *i_pName, size_t i_szLen) 
        { 
            bFunction = i_bFunction; 
            iBrakets  = 0; 
            pName     = NULL;
            if (i_szLen)
            {
                pName = (char*)malloc(i_szLen+1);
                if (pName)
                {
                    memcpy(pName, i_pName, i_szLen);
                    pName[i_szLen] = 0;
                }
            }
        }
        virtual ~stBlock() { if (pName) {free(pName); pName = NULL;}}
    };

    CpreManager       *m_pManager;
    tXCHAR            *m_pOsPath;
    char              *m_pDbPath; 
    tUINT8            *m_pData;
    size_t             m_szData;
    eModification      m_bModification;
    tUINT8             m_pHash[CKeccak::EBITS_256 / 8];
    CBList<CFuncRoot*> m_cFunctions;
    eErrorCodes        m_eError;
    tBOOL              m_bUpdated;
    CBList<stBlock*>   m_cBlocks;
    tBOOL              m_bIsVirtual;
    stFuncDesc        *m_pVirtMod;
    stFuncDesc        *m_pVirtTel;

public:
    CpreFile(CpreManager *i_pManager, const tXCHAR *i_pName, eErrorCodes &o_rError);
    CpreFile(CpreManager *i_pManager, const tXCHAR *i_pName, Cfg::INode *i_pNode, eErrorCodes &o_rError);
    virtual ~CpreFile();
    tBOOL                    IsVirtual()       { return m_bIsVirtual;    }
    const char              *GetPath()         { return m_pDbPath;       }
    const tXCHAR            *GetOsPath()       { return m_pOsPath;       }
    const tUINT8            *GetHash()         { return m_pHash;         }
    CpreFile::eModification  GetModification() { return m_bModification; }
    void                     SetModification(CpreFile::eModification i_eModification) { m_bModification = i_eModification; }
    tBOOL                    Save();
    void                     SetUpdated(tBOOL i_bUpdated) { m_bUpdated = i_bUpdated;}
    size_t                   GetTargetCpuBitsCount() { return m_pManager->GetTargetCpuBitsCount(); }
    size_t                   GetTargetCpuWCharBitsCount() { return m_pManager->GetTargetCpuWCharBitsCount(); }
    eErrorCodes              GetError();    
    void                     PrintErrors();
    CBList<CFuncRoot*>      &GetFunctions() { return m_cFunctions; }

private:
    tBOOL Parse();
    tBOOL IsKeyWord(const char* i_pHead, const char *i_pCurPos, const char **i_ppKeywords, size_t i_szKeywords);
    const char* GetFunctionName(const char* i_pHead, const char *i_pCurPos, size_t &o_rLength);
};

#endif //UP7_FILE_H
