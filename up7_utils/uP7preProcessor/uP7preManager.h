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
#ifndef UP7_MANAGER_H
#define UP7_MANAGER_H

class CpreFile; //forward definition

class CpreManager
{
    CFunctionsList       m_cFunctions;
    tXCHAR              *m_pSrcDir;
    tXCHAR              *m_pOutDir;
    tXCHAR              *m_pName;
    CBList<CpreFile*>    m_cFiles;
    CBList<CFuncTrace*>  m_cTraces;
    CBList<CFuncModule*> m_cModules;
    CBList<CFuncCounter*>m_cCounters;
    eErrorCodes          m_eError;
    size_t               m_szTargetCpuBits;
    size_t               m_szWcharBits;
    bool                 m_bIDsHeader;

public:
    CpreManager();
    virtual ~CpreManager();
    CFunctionsList& GetFunctions() {return m_cFunctions;}
    void            SetSourcesDir(const tXCHAR *i_pDir);
    void            SetOutputDir(const tXCHAR *i_pDir);
    const tXCHAR*   GetDir() { return m_pSrcDir;}
    void            SetName(const tXCHAR *i_pName);
    const tXCHAR*   GetName() { return m_pName; }
    void            SetTargetCpuBitsCount(size_t i_szBits);
    size_t          GetTargetCpuBitsCount() { return m_szTargetCpuBits; }
    void            SetTargetCpuWCharBitsCount(size_t i_szBits);
    size_t          GetTargetCpuWCharBitsCount() { return m_szWcharBits; }
    void            EnableIDsHeader();
    eErrorCodes     AddFile(const tXCHAR *i_pPath);
    eErrorCodes     AddFile(const tXCHAR *i_pPath, Cfg::INode *i_pNode);
    tBOOL           CheckFileHash(const tXCHAR *i_pName, const tXCHAR *i_pHash);
    tBOOL           SetReadOnlyFile(const tXCHAR *i_pName);
    tBOOL           SetExcludedFile(const tXCHAR *i_pName);

    int             Process();
    tBOOL           SaveHashes(Cfg::INode *i_pFiles);
private:
    eErrorCodes     CreateDescriptionHeaderFile(tUINT32      i_uSession, 
                                                tUINT8       i_uSessionCrc7,  
                                                tUINT64      i_qwEpochTime,  
                                                const tUINT8 i_pHash[CKeccak::EBITS_256 / 8],
                                                tBOOL        i_bUpdate
                                               );
    eErrorCodes     CreateIDsHeaderFile(tUINT32 i_uSession, tUINT64 i_qwEpochTime, tBOOL i_bUpdate);
    eErrorCodes     ParseDescriptionHeaderFile(tUINT32 &o_rSession, tUINT64 &o_rEpochTime, tUINT8 o_pSessionHash[CKeccak::EBITS_256 / 8]);
    eErrorCodes     CheckDuplicates();
    eErrorCodes     GenerateDefineName(const char *i_pName, char *o_pDefine, size_t i_szDefineMax);

};

#endif //UP7_MANAGER_H
