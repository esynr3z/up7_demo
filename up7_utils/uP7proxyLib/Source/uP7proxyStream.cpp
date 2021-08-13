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
#include "uP7common.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CProxyStream::CProxyStream(CWString      &i_rName, 
                           bool           i_bConvertEndianess,
                           CuP7Fifo      *i_pFifo,
                           uint8_t        i_bId,
                           uint64_t       i_qwFreq,
                           IuP7Time      *i_iTime,
                           IP7_Trace     *i_pP7Trace,
                           IP7_Telemetry *i_pP7Tel,
                           IProxyClient  *i_pClient
                          )
    : m_lReference(1)
    , m_cName(i_rName.Get())
    , m_bConvertEndianess(i_bConvertEndianess)
    , m_pP7Trace(i_pP7Trace)
    , m_hP7Mod(NULL)
    , m_pP7Tel(i_pP7Tel)
    , m_sTelTimeCorrection(P7TELEMETRY_INVALID_ID_V2)
    , m_pFifo(i_pFifo)
    , m_bId(i_bId)
    , m_eState(eStateDescription)
    , m_uSessionId(0)
    , m_uCrc7(0)
    , m_pProxyClient(i_pClient)
    , m_pClient(NULL)
    , m_uClientId(USER_PACKET_CHANNEL_ID_MAX_SIZE)
    , m_cPool(uP7_POOL_DEF_CHUNK_SIZE, uP7_POOL_DEF_MAX_SIZE)
    , m_bClosed(false)

    , m_qwCpuProxyCreationTime(0)
    , m_qwCpuStartTime(0)
    , m_qwCpuFreq(i_qwFreq)
    
    , m_qwHostProxyCreationTime(0)
    , m_qwHostFreq(GetPerformanceFrequency())

    , m_llTimeCorrection(0)
    , m_uOperationTimeStamp(0)
    , m_iTime(i_iTime)
    , m_bTimeInSync(false)
{
    LOCK_CREATE(m_hLock);

    m_stStatus.bConnected = TRUE;
    m_stStatus.dwResets   = 0;

    if (m_pFifo)
    {
        m_pFifo->Add_Ref();
    }

    if (m_iTime)
    {
        m_iTime->Add_Ref();
        m_bTimeInSync = true;
    }

    if (m_pP7Tel)
    {
        m_pP7Tel->Create(TM("Time correction"), -10, -10, 10, 10, TRUE, &m_sTelTimeCorrection);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CProxyStream::~CProxyStream()
{
    if (m_pClient)
    {
        m_pProxyClient->ReleaseChannel(m_pClient, m_uClientId);
        m_uClientId = USER_PACKET_CHANNEL_ID_MAX_SIZE;
        m_pClient   = NULL;
    }

    SAFE_RELEASE(m_pFifo);

    SAFE_RELEASE(m_iTime);

    LOCK_DESTROY(m_hLock);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CProxyStream::WaitTimeResponseAsync()
{
    LOCK_ENTER(m_hLock);
    if (eStateError != m_eState)
    {
        m_uOperationTimeStamp = GetTickCount();
        m_eState              = eStateWaitTimeResponse;
    }
    LOCK_EXIT(m_hLock);
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CProxyStream::Maintain()
{
    LOCK_ENTER(m_hLock);

    if (    (eStateWaitTimeResponse == m_eState)
         && (CTicks::Difference(GetTickCount(), m_uOperationTimeStamp) > TIME_SYNC_TIMEOUT_MS)
       )

    {
        uERROR(TM("[CPU#%d] Synchronization timeout. Timestamps won't be syncronized with HOST!"), (int)m_bId);
        Start();
    }
    else if (eStateReady == m_eState)
    {
        if (m_iTime)
        {
            //time passed for CPU
            uint64_t l_qwCpuDuration  = m_iTime->GetTime() - m_qwCpuStartTime; 
            //time passed for HOST with correction of command processing time
            uint64_t l_qwHostDuration = GetPerformanceCounter() - m_qwHostProxyCreationTime;   

            //uINFO(TM("[CPU#%d] Host duration %f, CPU Duration %f, Host roundtrip %f"), (int)m_bId ,
            //    (double)l_qwHostDuration / (double)m_qwHostFreq,
            //    (double)l_qwCpuDuration / (double)m_qwCpuFreq,
            //    (double)l_qwRoundTrip / (double)m_qwHostFreq);

            //converting to CPU timer frequency
            l_qwHostDuration = (uint64_t)((double)l_qwHostDuration * (double)m_qwCpuFreq / (double)m_qwHostFreq);

            m_llTimeCorrection = (int64_t)l_qwHostDuration - (int64_t)l_qwCpuDuration;

            if (    (m_llTimeCorrection < 0)
                 && (((int64_t)l_qwCpuDuration + m_llTimeCorrection) <= 0)
               )
            {
                uERROR(TM("[CPU#%d] Time correction is wrong!"), (int)m_bId);
                m_llTimeCorrection = 0;
            }

            if (    (m_pP7Tel)
                 && (P7TELEMETRY_INVALID_ID_V2 != m_sTelTimeCorrection)
                )
            {
                m_pP7Tel->Add(m_sTelTimeCorrection, (double)m_llTimeCorrection/(double)m_qwCpuFreq);
            }
        }
    }

    LOCK_EXIT(m_hLock);

    return false;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CProxyStream::SendChunks()
{
    bool l_bReturn = true;

    if (!m_pChunksHead)
    {
        return false;
    }

    if (m_stStatus.bConnected)
    {
        //sending first prev. accumulated data
        while (    (m_pChunksHead) 
                && (m_pChunksHead->szChunks)
              )
        {
            if (ECLIENT_STATUS_OK == m_pClient->Sent(m_uClientId, 
                                                     m_pChunksHead->pChunks, 
                                                     (tUINT32)m_pChunksHead->szChunks, 
                                                     (tUINT32)m_pChunksHead->szData)
               )
            {
                ClearChunkHead();
            }
            else
            {
                l_bReturn = false;
                break;
            }
        }

        //it all service data has been sent - trying to send data from CPU
        if (!m_pChunksHead->szChunks)
        {
            CLinearPool::stBuffer *l_pBuf = NULL;
            while ((l_pBuf = m_cPool.PullUsed()))
            {
                sP7C_Data_Chunk l_stChunk = {l_pBuf->pData, (tUINT32)l_pBuf->szData};
                if (ECLIENT_STATUS_OK != m_pClient->Sent(m_uClientId, &l_stChunk, 1, l_stChunk.dwSize))
                {
                    m_cPool.PushFree(l_pBuf);
                    l_bReturn = false;
                    break;
                }

                m_cPool.PushFree(l_pBuf);
            }
        }
    }

    return l_bReturn;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t CProxyStream::Add_Ref()
{
    return ATOMIC_INC(&m_lReference);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t CProxyStream::Release()
{
    tINT32 l_lResult = ATOMIC_DEC(&m_lReference);
    if ( 0 >= l_lResult )
    {
        delete this;
    }

    return l_lResult;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CProxyStream::On_Init(sP7C_Channel_Info *i_pInfo) 
{
    LOCK_ENTER(m_hLock);
    if (i_pInfo)
    {
        m_uClientId = i_pInfo->dwID;
    }
    LOCK_EXIT(m_hLock);
}


