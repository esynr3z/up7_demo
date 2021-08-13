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
#ifndef UP7_SESSION_FILE_H
#define UP7_SESSION_FILE_H

class CSessionFile
{
private:
    size_t                  m_szData;
    size_t                  m_szDataMax;
    tUINT8                 *m_pData; 
    tUINT8                  m_pHash[CKeccak::EBITS_256 / 8];
    stuP7SessionFileHeader *m_pHdr;
    eErrorCodes             m_eError;

public:
    CSessionFile(const tXCHAR *i_pName);
    CSessionFile(CBList<CpreFile*> *i_pFiles, tUINT32  i_uSession, uint64_t i_qwEpochTime);
    virtual ~CSessionFile();

    eErrorCodes SetSession(uint32_t  i_uSession, uint8_t i_uSessionCrc7, uint64_t i_qwEpochTime);
    eErrorCodes Save(const tXCHAR *i_pName);

    const stuP7SessionFileHeader *GetHeader() { return m_pHdr;   }
    eErrorCodes                   GetError()  { return m_eError; }
    const tUINT8                 *GetHash()   { return m_pHash;  }

private: 
    void UpdateHash();
};

#endif //UP7_SESSION_FILE_H
