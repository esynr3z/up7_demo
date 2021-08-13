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

#define DEFULT_HOST_CPU_FIFO_SIZE 8192


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//CAList::Data_Release
// override this function in derived class to implement specific data 
// destructor(structures as example)
tBOOL CFifoList::Data_Release(CuP7Fifo *i_pData)
{
    if (NULL == i_pData)
    {
        return FALSE;
    }

    i_pData->Release();
    return TRUE;
}//CAList::Data_Release


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CuP7FifoGroup::CuP7FifoGroup(CMEvent *i_pWriteEvent, tUINT32 i_uWriteEventId)
    : m_pWriteEvent(i_pWriteEvent)
    , m_uWriteEventId(i_uWriteEventId)
{
    LOCK_CREATE(m_hCS);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CuP7FifoGroup::~CuP7FifoGroup()
{
    m_cFifos.Clear(TRUE);
    LOCK_DESTROY(m_hCS);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CuP7FifoGroup::RegisterFifo(CuP7Fifo *i_pFifo)
{
    if (!i_pFifo)
    {
        return false;
    }

    LOCK_ENTER(m_hCS);
    pAList_Cell l_pEl    = NULL;
    while ((l_pEl = m_cFifos.Get_Next(l_pEl)))
    {
        if (i_pFifo == m_cFifos.Get_Data(l_pEl))
        {
            break;
        }
    }

    bool l_bSetGroup = false;

    if (!l_pEl)
    {
        l_pEl = m_cFifos.Push_Last(i_pFifo);
        if (l_pEl)
        {
            i_pFifo->Add_Ref();
            l_bSetGroup = true;
        }
    }
    LOCK_EXIT(m_hCS);

    if (l_bSetGroup)
    {
        i_pFifo->SetGroup(this);
    }

    return l_pEl ? true : false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CuP7FifoGroup::UnregisterFifo(CuP7Fifo *i_pFifo)
{
    if (!i_pFifo)
    {
        return false;
    }

    LOCK_ENTER(m_hCS);
    pAList_Cell l_pEl = NULL;
    while ((l_pEl = m_cFifos.Get_Next(l_pEl)))
    {
        if (i_pFifo == m_cFifos.Get_Data(l_pEl))
        {
            m_cFifos.Del(l_pEl, FALSE);
            break;
        }
    }
    LOCK_EXIT(m_hCS);

    if (l_pEl)
    {
        i_pFifo->SetGroup(NULL);
    }

    return l_pEl ? true : false;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CuP7Fifo::stBuffer *CuP7FifoGroup::PullBuffer()
{
    CuP7Fifo::stBuffer *l_pReturn = NULL;
    LOCK_ENTER(m_hCS);
    if (m_cCpu2Host.Count())
    {
        l_pReturn = m_cCpu2Host.Pull_First();
    }
    LOCK_EXIT(m_hCS);

    return l_pReturn;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CuP7Fifo::CuP7Fifo(uint8_t i_bCpuId, size_t i_szFifoSize, bool i_bFifoBiDirectional, IP7_Trace *i_pP7Trace)
    : m_lReference(1)
    , m_bCpuId(i_bCpuId)
    , m_pHost2Cpu(i_bFifoBiDirectional ? new CRingBuffer(DEFULT_HOST_CPU_FIFO_SIZE) : NULL)
    , m_pGroup(NULL)
    , m_pP7Trace(i_pP7Trace)
    , m_hP7Mod()
    , m_cCpu2HostCurrent(NULL)
    , m_szCpu2HostBuffer(0)
    , m_szCpu2Host(0)
    , m_szCpu2HostFree(0)

{
    LOCK_CREATE(m_hCS);
    m_cReadEvent.Init(1, EMEVENT_MULTI);

    if (m_pP7Trace)
    {
        m_pP7Trace->Add_Ref();
        m_pP7Trace->Register_Module(TM("Fifo"), &m_hP7Mod);
    }

    size_t l_szBufferSize   = FIFO_MAX_BUFFER_SIZE * 2;
    size_t l_dwBuffersCount = 0;

    ////////////////////////////////////////////////////////////////////////////
    //calculate buffer size and buffers count
    //Starting from largest buffer size trying to calculate how much buffers we
    //can allocate, if amount is smaller than minimum - device buffer size by 2
    //and repeat procedure
    while (l_dwBuffersCount < FIFO_MIN_BUFFERS_COUNT)
    {
        l_szBufferSize /= 2; 
        l_dwBuffersCount = i_szFifoSize / l_szBufferSize;
    }

    m_szCpu2HostBuffer = l_szBufferSize;
    m_szCpu2Host       = l_dwBuffersCount * l_szBufferSize;
    m_szCpu2HostFree   = m_szCpu2Host;

    while (l_dwBuffersCount--)
    {
        m_cCpu2Host.Push_Last(new stBuffer(m_bCpuId, l_szBufferSize));
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CuP7Fifo::~CuP7Fifo()
{
    m_cCpu2Host.Clear(TRUE);
    if (m_cCpu2HostCurrent)
    {
        delete m_cCpu2HostCurrent;
        m_cCpu2HostCurrent = NULL;
    }


    if (m_pHost2Cpu)
    {
        delete m_pHost2Cpu;
        m_pHost2Cpu = NULL;
    }

    SAFE_RELEASE(m_pP7Trace);

    LOCK_DESTROY(m_hCS);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CuP7Fifo::Write(const void *i_pData, size_t i_szData)
{
    if (    (!i_pData)
         || (!i_szData)
       )
    {
        return false;
    }

    LOCK_ENTER(m_hCS);
    bool l_bReturn = false;

    if (i_szData < m_szCpu2HostFree)
    {
        bool   l_bExit        = false;
        size_t l_szChunkOffs = 0;
        size_t l_szChunkSize = i_szData;
    
        m_szCpu2HostFree -= i_szData;

        while (FALSE == l_bExit)
        {
            //if packet is null we need to extract another one 
            if (NULL == m_cCpu2HostCurrent)
            {
                m_cCpu2HostCurrent = m_cCpu2Host.Pull_First();
            }

            while (    (m_cCpu2HostCurrent)
                    && (l_szChunkSize)
                  )
            {
                //if packet free size is larger or equal to chunk size
                if ( BUFFER_TAIL_SIZE(m_cCpu2HostCurrent) >= l_szChunkSize )
                {
                    memcpy(m_cCpu2HostCurrent->pData + m_cCpu2HostCurrent->szUsed, 
                           ((uint8_t*)i_pData) + l_szChunkOffs,
                           l_szChunkSize
                          );

                    m_cCpu2HostCurrent->szUsed += l_szChunkSize;

                    l_szChunkSize = 0;
                    l_bExit = true;

                    //if packet is filled - put it to data queue
                    if (0 >= BUFFER_TAIL_SIZE(m_cCpu2HostCurrent))
                    {
                        LOCK_ENTER(m_pGroup->m_hCS);
                        m_pGroup->m_cCpu2Host.Push_Last(m_cCpu2HostCurrent);
                        m_pGroup->m_pWriteEvent->Set(m_pGroup->m_uWriteEventId);
                        LOCK_EXIT(m_pGroup->m_hCS);

                        m_cCpu2HostCurrent = NULL;
                    }

                }
                else //if chunk data is greater than packet free space
                {
                    memcpy(m_cCpu2HostCurrent->pData + m_cCpu2HostCurrent->szUsed, 
                           ((uint8_t*)i_pData) + l_szChunkOffs,
                           BUFFER_TAIL_SIZE(m_cCpu2HostCurrent)
                          );

                    l_szChunkOffs              += BUFFER_TAIL_SIZE(m_cCpu2HostCurrent);
                    l_szChunkSize              -= BUFFER_TAIL_SIZE(m_cCpu2HostCurrent);
                    m_cCpu2HostCurrent->szUsed += BUFFER_TAIL_SIZE(m_cCpu2HostCurrent);

                    LOCK_ENTER(m_pGroup->m_hCS);
                    m_pGroup->m_cCpu2Host.Push_Last(m_cCpu2HostCurrent);
                    m_pGroup->m_pWriteEvent->Set(m_pGroup->m_uWriteEventId);
                    LOCK_EXIT(m_pGroup->m_hCS);

                    m_cCpu2HostCurrent = NULL;
                }
            } //while ( (m_pHost2Cpu2Current) && (i_szChunks) )
        } //while (FALSE == l_bExit)

        l_bReturn = true;
    }
    else
    {
        uERROR(TM("[%u] Drop incoming data %zu, not enough free space in buffer %zu"), 
               (uint32_t)m_bCpuId, 
               i_szData, 
               m_szCpu2HostFree);
    }
    LOCK_EXIT(m_hCS);

    return l_bReturn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CuP7Fifo::Write(const IuP7Fifo::stChunk *i_pChunks, size_t i_szChunks, size_t i_szData)
{
    if (    (!i_pChunks)
         || (!i_szChunks)
         || (!i_szData)
       )
    {
        return false;
    }

    LOCK_ENTER(m_hCS);
    bool l_bReturn = false;

    if (i_szData < m_szCpu2HostFree)
    {
        bool   l_bExit        = false;
        size_t l_szChunkOffs = 0;
        size_t l_szChunkSize = i_pChunks->szData;
    
        m_szCpu2HostFree -= i_szData;

        while (FALSE == l_bExit)
        {
            //if packet is null we need to extract another one 
            if (NULL == m_cCpu2HostCurrent)
            {
                m_cCpu2HostCurrent = m_cCpu2Host.Pull_First();
            }

            while (    (m_cCpu2HostCurrent)
                    && (i_szChunks)
                  )
            {
                //if packet free size is larger or equal to chunk size
                if ( BUFFER_TAIL_SIZE(m_cCpu2HostCurrent) >= l_szChunkSize )
                {
                    memcpy(m_cCpu2HostCurrent->pData + m_cCpu2HostCurrent->szUsed, 
                           ((uint8_t*)i_pChunks->pData) + l_szChunkOffs,
                           l_szChunkSize
                          );

                    m_cCpu2HostCurrent->szUsed += l_szChunkSize;

                    //current chunk was moved, we reduce chunks amount 
                    i_szChunks--;

                    //we are finish with that chunk
                    //l_szChunkSize = 0; 
                    l_szChunkOffs = 0;

                    if (i_szChunks > 0)
                    {
                        //go to next chunk
                        i_pChunks ++;
                        l_szChunkSize = i_pChunks->szData;
                    }
                    else
                    {
                        l_bExit = true;
                    }

                    //if packet is filled - put it to data queue
                    if (0 >= BUFFER_TAIL_SIZE(m_cCpu2HostCurrent))
                    {
                        LOCK_ENTER(m_pGroup->m_hCS);
                        m_pGroup->m_cCpu2Host.Push_Last(m_cCpu2HostCurrent);
                        m_pGroup->m_pWriteEvent->Set(m_pGroup->m_uWriteEventId);
                        LOCK_EXIT(m_pGroup->m_hCS);

                        m_cCpu2HostCurrent = NULL;
                    }

                }
                else //if chunk data is greater than packet free space
                {
                    memcpy(m_cCpu2HostCurrent->pData + m_cCpu2HostCurrent->szUsed, 
                           ((uint8_t*)i_pChunks->pData) + l_szChunkOffs,
                           BUFFER_TAIL_SIZE(m_cCpu2HostCurrent)
                          );

                    l_szChunkOffs              += BUFFER_TAIL_SIZE(m_cCpu2HostCurrent);
                    l_szChunkSize              -= BUFFER_TAIL_SIZE(m_cCpu2HostCurrent);
                    m_cCpu2HostCurrent->szUsed += BUFFER_TAIL_SIZE(m_cCpu2HostCurrent);

                    LOCK_ENTER(m_pGroup->m_hCS);
                    m_pGroup->m_cCpu2Host.Push_Last(m_cCpu2HostCurrent);
                    m_pGroup->m_pWriteEvent->Set(m_pGroup->m_uWriteEventId);
                    LOCK_EXIT(m_pGroup->m_hCS);

                    m_cCpu2HostCurrent = NULL;
                }
            } //while ( (m_cCpu2HostCurrent) && (i_szChunks) )
        } //while (FALSE == l_bExit)

        l_bReturn = true;
    }
    else
    {
        uERROR(TM("[%u] Drop incoming data %zu, not enough free space in buffer %zu"), 
               (uint32_t)m_bCpuId, 
               i_szData, 
               m_szCpu2HostFree);
    }

    LOCK_EXIT(m_hCS);

    return l_bReturn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
size_t CuP7Fifo::GetFreeBytes()
{
    LOCK_ENTER(m_hCS);
    size_t l_szReturn = m_szCpu2HostFree;
    LOCK_EXIT(m_hCS);

    return l_szReturn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
size_t CuP7Fifo::Read(void *i_pBuffer, size_t i_szBuffer, uint32_t i_uTimeout) //<m_cHost2Cpu, User ca
{
    uint32_t l_uStartTime  = GetTickCount();
    uint32_t l_uCorrection = 0;
    size_t   l_szUsed      = 0;

    if (    (!i_pBuffer)
         || (!i_szBuffer)
         || (!m_pHost2Cpu)
       )
    {
        return 0;
    }


    while (    (MEVENT_TIME_OUT != m_cReadEvent.Wait(i_uTimeout - l_uCorrection))
            && (!l_szUsed)
          )
    {
        LOCK_ENTER(m_hCS);
        l_szUsed = GET_MIN(m_pHost2Cpu->GetUsedSize(), i_szBuffer);
        if (l_szUsed)
        {
            if (!m_pHost2Cpu->Read((tUINT8*)i_pBuffer, l_szUsed))
            {
                l_szUsed = 0;
            }
        }
        else
        {
            l_uCorrection = CTicks::Difference(GetTickCount(), l_uStartTime);
            if (l_uCorrection > i_uTimeout)
            {
                l_uCorrection = i_uTimeout;
            }
        }

        LOCK_EXIT(m_hCS);
    }

    return l_szUsed;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CuP7Fifo::Send(const void *i_pData, size_t i_szData) //>m_cHost2Cpu
{
    if (    (!i_pData)
         || (!i_szData)
         || (!m_pHost2Cpu)
       )
    {
        return false;
    }

    LOCK_ENTER(m_hCS);
    bool l_bReturn = m_pHost2Cpu->Write((tUINT8*)i_pData, i_szData);
    LOCK_EXIT(m_hCS);

    if (l_bReturn)
    {
        m_cReadEvent.Set(0);
    }
    else
    {
        uERROR(TM("[%u] Write %zu data failed"), 
               (uint32_t)m_bCpuId, 
               i_szData);
    }

    return l_bReturn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CuP7Fifo::SetGroup(CuP7FifoGroup *i_pGroup)
{
    LOCK_ENTER(m_hCS);
    m_pGroup = i_pGroup;
    LOCK_EXIT(m_hCS);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CuP7Fifo::PushBuffer(CuP7Fifo::stBuffer *i_pBuffer)
{
    LOCK_ENTER(m_hCS);
    m_szCpu2HostFree += m_szCpu2HostBuffer;
    i_pBuffer->szUsed = 0;
    m_cCpu2Host.Push_Last(i_pBuffer);
    LOCK_EXIT(m_hCS);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CuP7Fifo::stBuffer *CuP7Fifo::PullFirst()
{
    CuP7Fifo::stBuffer *l_pReturn = NULL;
    LOCK_ENTER(m_hCS);
    if (m_cCpu2HostCurrent)
    {
        m_szCpu2HostFree -= m_szCpu2HostBuffer - m_cCpu2HostCurrent->szUsed;

        l_pReturn = m_cCpu2HostCurrent;
        m_cCpu2HostCurrent = NULL;
    }
    LOCK_EXIT(m_hCS);

    return l_pReturn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t CuP7Fifo::Add_Ref()
{
    return ATOMIC_INC(&m_lReference);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t CuP7Fifo::Release()
{
    tINT32 l_lResult = ATOMIC_DEC(&m_lReference);
    if ( 0 >= l_lResult )
    {
        delete this;
    }

    return l_lResult;
}


