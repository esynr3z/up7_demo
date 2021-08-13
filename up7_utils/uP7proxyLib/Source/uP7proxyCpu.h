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
#ifndef UP7_PROXY_CPU_H
#define UP7_PROXY_CPU_H


#define uP7_CPU_STREAMS_COUNT 2

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CProxyCpu
{
    enum eState
    {
        eSessionRecognition,
        eSessionResync,
        eProcessing,
        eError
    };

    struct stStream
    {
        IProxyStream  *iStream;
        stProxyPacket *pFirst;
        stProxyPacket *pLast;
        uint64_t       qwPacketsMap;
    };

    const static size_t m_uBlockSize = 128;

    struct stBlock
    {
        stBlock(stBlock *pPrev) {pNext = pPrev;}

        stProxyPacket pPackets[m_uBlockSize];
        stBlock *pNext;
    };


protected:
    CWString                m_cName;
    stStream                m_cStreams[uP7_CPU_STREAMS_COUNT];
    eState                  m_cObjState;
    bool                    m_bError;
    bool                    m_bConvertEndianess;
    IP7_Trace              *m_pP7Trace;
    IP7_Trace::hModule      m_hP7Mod;
    IP7_Telemetry          *m_pP7Tel;
    CPreProcessorFilesTree *m_pFiles;
    CuP7Fifo               *m_iFifo;
    uint8_t                 m_bId;

    uint64_t                m_qwUnknownData;

    uint8_t                *m_pTail;
    size_t                  m_szTail;
    size_t                  m_szTailRest;
    stuP7baseHdr           *m_pTailHdr;

    stBlock                *m_pBlockPackets; 
    stProxyPacket          *m_pFreePackets;

    uint32_t                m_uSessionId;
    uint8_t                 m_uCrc7;

    IProxyClient           *m_pClient;

    uint64_t                m_qwFreq;

    IuP7Time               *m_iTime;


public:
    CProxyCpu(CWString               &i_rName, 
              bool                    i_bConvertEndianess,
              CPreProcessorFilesTree *i_pFiles,
              CuP7Fifo               *i_pFifo,
              uint8_t                 i_bId,
              uint64_t                i_qwFreq, 
              IuP7Time               *i_iTime,
              IP7_Trace              *i_pP7Trace,
              IP7_Telemetry          *i_pP7Tel,
              IProxyClient           *i_pClient
             );
    virtual ~CProxyCpu();
    bool     Process(CuP7Fifo::stBuffer *i_pBuffer);
    bool     Maintain();
    uint8_t  GetId() {return m_bId;}
protected:
    bool                      SyncronizeSession(const uint8_t *i_pData, size_t i_szData);
    const stPreProcessorFile *FindDescription(uint32_t i_uSessionId);
    void                      AddBlockPackets();
    void                      AddPacket(stStream &i_rStream, stuP7baseHdr *i_pHdr);
    bool                      ApplyDescription(uint32_t i_uSessionId);

    inline void ReleaseStreamsPackets()
    {
        //return packets to pool
        for (size_t l_szI = 0; l_szI < uP7_CPU_STREAMS_COUNT; l_szI++)
        {
            if (    (m_cStreams[l_szI].pFirst)
                 && (m_cStreams[l_szI].pLast)
               )
            {
                m_cStreams[l_szI].pLast->pNext = m_pFreePackets;
                m_pFreePackets = m_cStreams[l_szI].pFirst;
            }

            m_cStreams[l_szI].pFirst = NULL;
            m_cStreams[l_szI].pLast  = NULL;
        }
    }
};


#endif //UP7_PROXY_CPU_H