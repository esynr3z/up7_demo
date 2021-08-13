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
#ifndef UP7_PROXY_STREAM_H
#define UP7_PROXY_STREAM_H

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct stProxyPacket
{
    stuP7baseHdr  *pHdr;
    stProxyPacket *pNext;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CTimeList: public CListPool<int64_t>
{
public:
    CTimeList() : CListPool(16) {}
protected:
    virtual tBOOL Data_Release(int64_t i_pData) { UNUSED_ARG(i_pData); return TRUE; }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IProxyStream
{
public:
    virtual void     WaitTimeResponseAsync() = 0;
    virtual void     UpdateStartTime(const stProxyPacket *i_pPackets) = 0;
    virtual bool     ParseDescription(uint32_t i_uSessionId, uint8_t i_uCrc7, const uint8_t *i_pDesc, size_t i_szDesc)  = 0;
    virtual size_t   Process(const stProxyPacket *i_pPackets)  = 0;
    virtual uint64_t GetSupportedPacketsMap() = 0;
    virtual bool     Maintain() = 0;
    virtual void     Start() = 0;
    virtual int32_t  Add_Ref() = 0;
    virtual int32_t  Release() = 0;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CProxyStream
    : public virtual IProxyStream
    , public virtual IP7C_Channel    
    , public virtual CChunks
{            
protected:
    enum eState
    {
        eStateError = -1,  //ORDER IS IMPORTANT!
        eStateDescription, 
        eStateCommunicationIsOn,
        eStateWaitTimeResponse = eStateCommunicationIsOn,
        eStateReady
    };

    //put volatile variables at the top, to obtain 32 bit alignment. 
    //Project has 8 bytes alignment by default
    tINT32 volatile        m_lReference;
                          
    CWString               m_cName;
    bool                   m_bConvertEndianess;
    IP7_Trace             *m_pP7Trace;
    IP7_Trace::hModule     m_hP7Mod;
    IP7_Telemetry         *m_pP7Tel;
    tUINT16                m_sTelTimeCorrection;
    CuP7Fifo              *m_pFifo;
    uint8_t                m_bId;
    eState                 m_eState;
    uint32_t               m_uSessionId;
    uint8_t                m_uCrc7;
                          
    IProxyClient          *m_pProxyClient;
    IP7_Client            *m_pClient;
    uint32_t               m_uClientId;
    sP7C_Status            m_stStatus;
                          
    CLinearPool            m_cPool;

    tLOCK                  m_hLock;

    bool                   m_bClosed;


    uint64_t               m_qwCpuProxyCreationTime;
    uint64_t               m_qwCpuStartTime;
    uint64_t               m_qwCpuFreq;

    uint64_t               m_qwHostProxyCreationTime;
    uint64_t               m_qwHostFreq;

    int64_t                m_llTimeCorrection;

    uint32_t               m_uOperationTimeStamp;

    IuP7Time              *m_iTime;

    bool                   m_bTimeInSync;
public:
    CProxyStream(CWString      &i_rName, 
                 bool           i_bConvertEndianess,
                 CuP7Fifo      *i_pFifo,
                 uint8_t        i_bId,
                 uint64_t       i_qwFreq,
                 IuP7Time      *i_iTime,
                 IP7_Trace     *i_pP7Trace,
                 IP7_Telemetry *i_pP7Tel,
                 IProxyClient  *i_pClient
                );

    virtual ~CProxyStream();
    void     WaitTimeResponseAsync();
    bool     Maintain();
    bool     SendChunks();
    int32_t  Add_Ref();
    int32_t  Release();

    void     On_Init(sP7C_Channel_Info *i_pInfo);

    template<typename P7CreationHdrType>
    void AllignSessionTime(P7CreationHdrType o_pP7hdr)
    {
        double   l_dbCpuTimeDiff  = (double)m_qwCpuProxyCreationTime - (double)m_qwCpuStartTime;
        double   l_dbAbsTimeDiff  = l_dbCpuTimeDiff * (double)TIME_SEC_100NS / (double)m_qwCpuFreq;
        double   l_dbHostTimeDiff = l_dbCpuTimeDiff * (double)m_qwHostFreq / (double)m_qwCpuFreq;
        uint64_t l_qwChannelTime  = (((uint64_t)o_pP7hdr->dwTime_Hi) << 32) + (uint64_t)o_pP7hdr->dwTime_Lo;

        l_qwChannelTime     = (uint64_t)((int64_t)l_qwChannelTime - (int64_t)l_dbAbsTimeDiff);
        o_pP7hdr->dwTime_Hi = (uint32_t)((l_qwChannelTime >> 32) & 0xFFFFFFFFull);
        o_pP7hdr->dwTime_Lo = (uint32_t)(l_qwChannelTime & 0xFFFFFFFFull);

        m_qwHostProxyCreationTime = (uint64_t)((int64_t)m_qwHostProxyCreationTime - (int64_t)l_dbHostTimeDiff);

        m_eState = eStateReady;
    }
};

#endif //UP7_PROXY_STREAM_H