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

static_assert(uP7_MAX_PACKET_SIZE < FIFO_MAX_BUFFER_SIZE, "FIFO buffer size is too small to store packet");


#define CPU_MAX_SESSION_UNK_SIZE (uP7_MAX_PACKET_SIZE) * 4


#define PROCESS_STREAMS_DATA()\
    for (size_t l_szI = 0; l_szI < uP7_CPU_STREAMS_COUNT; l_szI++)\
    {\
        if (m_cStreams[l_szI].pFirst)\
        {\
            m_cStreams[l_szI].iStream->Process(m_cStreams[l_szI].pFirst);\
        }\
    }



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CProxyCpu::CProxyCpu(CWString               &i_rName, 
                     bool                    i_bConvertEndianess,
                     CPreProcessorFilesTree *i_pFiles,
                     CuP7Fifo               *i_pFifo,
                     uint8_t                 i_bId,
                     uint64_t                i_qwFreq, 
                     IuP7Time               *i_iTime,
                     IP7_Trace              *i_pP7Trace,
                     IP7_Telemetry          *i_pP7Tel,
                     IProxyClient           *i_pClient
                    )
    : m_cName(i_rName.Get())
    , m_cObjState(eState::eSessionRecognition)
    , m_bError(false)
    , m_bConvertEndianess(i_bConvertEndianess)
    , m_pP7Trace(i_pP7Trace)
    , m_hP7Mod(NULL)
    , m_pP7Tel(i_pP7Tel)
    , m_pFiles(i_pFiles)
    , m_iFifo(i_pFifo)
    , m_bId(i_bId)
    , m_qwUnknownData(0ull)
    , m_pTail(0)
    , m_szTail(0)
    , m_pTailHdr(0)
    , m_pBlockPackets(0)
    , m_pFreePackets(0)
    , m_uSessionId(uP7_PROTOCOL_SESSION_UNK)
    , m_uCrc7(uP7_PROTOCOL_SESSION_CRC_UNK)
    , m_pClient(i_pClient)
    , m_qwFreq(i_qwFreq)
    , m_iTime(i_iTime)
{
    if (m_pP7Trace)
    {
        m_pP7Trace->Register_Module(TM("ProxyCpu"), &m_hP7Mod);
    }

    if (m_iFifo)
    {
        m_iFifo->Add_Ref();
    }

    if (m_iTime)
    {
        m_iTime->Add_Ref();
    }

    static_assert(uP7_CPU_STREAMS_COUNT == 2, "Not all streams are initialized");

    m_cStreams[0].iStream      = new CProxyTrace(m_cName, m_bConvertEndianess, i_pFifo, m_bId, i_qwFreq, m_iTime, m_pP7Trace, m_pP7Tel, m_pClient);
    m_cStreams[0].pFirst       = NULL;
    m_cStreams[0].pLast        = NULL;
    m_cStreams[0].qwPacketsMap = m_cStreams[0].iStream->GetSupportedPacketsMap();

    m_cStreams[1].iStream      = new CProxyTelemetry(m_cName, m_bConvertEndianess, i_pFifo, m_bId, i_qwFreq, m_iTime, m_pP7Trace, m_pP7Tel, m_pClient);
    m_cStreams[1].pFirst       = NULL;
    m_cStreams[1].pLast        = NULL;
    m_cStreams[1].qwPacketsMap = m_cStreams[1].iStream->GetSupportedPacketsMap();

    m_pTail       = (uint8_t*)malloc(FIFO_MAX_BUFFER_SIZE); //max theoretical packet size
    m_szTail      = 0;
    m_szTailRest  = 0;
    m_pTailHdr    = (stuP7baseHdr*)m_pTail;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CProxyCpu::~CProxyCpu()
{
    for (size_t l_szI = 0; l_szI < uP7_CPU_STREAMS_COUNT; l_szI++)
    {
        SAFE_RELEASE(m_cStreams[l_szI].iStream);
        m_cStreams[l_szI].pFirst = NULL;
        m_cStreams[l_szI].pLast  = NULL;
    }

    if (m_iFifo)
    {
        m_iFifo->Release();
        m_iFifo = NULL;
    }

    if (m_iTime)
    {
        m_iTime->Release();
        m_iTime = NULL;
    }

    if (m_pTail)
    {
        free(m_pTail);
        m_pTail = NULL;
    }

    while (m_pBlockPackets)
    {
        stBlock *l_pCurrent = m_pBlockPackets;
        m_pBlockPackets = m_pBlockPackets->pNext;
        delete l_pCurrent;
    }

    m_pTailHdr = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CProxyCpu::Process(CuP7Fifo::stBuffer *i_pBuffer)
{
    if (eState::eError == m_cObjState)
    {
        return false;
    }

    uint8_t *l_pData          = i_pBuffer->pData;
    size_t   l_szData         = i_pBuffer->szUsed;
    size_t   l_szToCopy       = 0;
    bool     l_bReturn        = true;

    if (!l_pData)
    {
        return false;
    }

    if (eState::eProcessing == m_cObjState)
    {
        //fill tail
        while (    (l_szData)
                && (m_szTailRest)
              )
        {
            //if tail is bigger that header - we need to update the remainder
            l_szToCopy = GET_MIN(m_szTailRest, l_szData);


            memcpy(m_pTail + m_szTail, l_pData, l_szToCopy);
            m_szTail     += l_szToCopy;
            l_pData      += l_szToCopy;
            l_szData     -= l_szToCopy;
            m_szTailRest -= l_szToCopy;

            if (    (!m_szTailRest)
                 && (sizeof(stuP7baseHdr) == m_szTail) //if we just received the header
               )
            {
                if (m_bConvertEndianess)
                {
                    m_pTailHdr->uSessionId = ntohl(m_pTailHdr->uSessionId);
                    m_pTailHdr->wSize      = ntohs(m_pTailHdr->wSize);
                }

                if (m_pTailHdr->uSessionId == m_uSessionId)
                {
                    if (m_pTailHdr->wSize > m_szTail)
                    {
                        m_szTailRest = m_pTailHdr->wSize - m_szTail;
                    }
                }
                else
                {
                    if (m_bConvertEndianess) //convert back
                    {
                        m_pTailHdr->uSessionId = ntohl(m_pTailHdr->uSessionId);
                        m_pTailHdr->wSize      = ntohs(m_pTailHdr->wSize);
                    }


                    uERROR(TM("[CPU#%d] Steam discontinuity is detected, trying to resync"), (int)m_bId); 

                    m_szTail     = 0;
                    m_szTailRest = 0;

                    m_cObjState  = eState::eSessionResync;
                    l_bReturn    = SyncronizeSession(i_pBuffer->pData, i_pBuffer->szUsed);
                    goto l_lblExit;
                }
            }
        }

    
        if (m_szTail) //if there is a tail to process 
        {
            if (!m_szTailRest) //and tail is full   
            {                                 
                uint64_t l_qwMask = 1ull << m_pTailHdr->bType;
                for (size_t l_szI = 0; l_szI < uP7_CPU_STREAMS_COUNT; l_szI++)
                {
                    if (l_qwMask & m_cStreams[l_szI].qwPacketsMap)
                    {
                        AddPacket(m_cStreams[l_szI], m_pTailHdr);
                    }
                }

                m_szTail = 0;
            }
            else //there is still data to add to tail ...
            {
                goto l_lblExit;
            }
        }

        //process data
        while (l_szData >= sizeof(stuP7baseHdr))
        {
            stuP7baseHdr *pHdr = (stuP7baseHdr*)l_pData;

            if (m_bConvertEndianess)
            {
                pHdr->uSessionId = ntohl(pHdr->uSessionId);
                pHdr->wSize      = ntohs(pHdr->wSize);
            }

            if (pHdr->uSessionId == m_uSessionId)
            {
                if (l_szData >= pHdr->wSize)
                {
                    uint64_t l_qwMask = 1ull << pHdr->bType;
                    for (size_t l_szI = 0; l_szI < uP7_CPU_STREAMS_COUNT; l_szI++)
                    {
                        if (l_qwMask & m_cStreams[l_szI].qwPacketsMap)
                        {
                            AddPacket(m_cStreams[l_szI], pHdr);
                        }
                    }

                    l_szData -= pHdr->wSize;
                    l_pData  += pHdr->wSize;
                }
                else
                {
                    break;
                }
            }
            else
            {
                PROCESS_STREAMS_DATA();

                ReleaseStreamsPackets();
    
                if (m_bConvertEndianess) //convert back
                {
                    pHdr->uSessionId = ntohl(pHdr->uSessionId);
                    pHdr->wSize      = ntohs(pHdr->wSize);
                }

                uERROR(TM("[CPU#%d] Steam discontinuity is detected, trying to resync"), (int)m_bId); 

                m_cObjState = eState::eSessionResync;
                l_bReturn = SyncronizeSession(l_pData, l_szData);
                goto l_lblExit;
            }

        }

        //process data
        PROCESS_STREAMS_DATA();

        if (l_szData) //there is a tail ... add for further processing
        {
            memcpy(m_pTail, l_pData, l_szData);
            m_szTail = l_szData;

            if (l_szData >= sizeof(stuP7baseHdr))
            {
                m_szTailRest = (size_t)m_pTailHdr->wSize - l_szData;
            }
            else
            {
                m_szTailRest = sizeof(stuP7baseHdr) - l_szData;
            }
        }

    }
    else if (    (eState::eSessionRecognition == m_cObjState)
              || (eState::eSessionResync == m_cObjState)
            )
    {
        l_bReturn = SyncronizeSession(l_pData, l_szData);
    }

    
l_lblExit:
    ReleaseStreamsPackets();

    //release fifo buffer
    m_iFifo->PushBuffer(i_pBuffer);
    return l_bReturn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CProxyCpu::SyncronizeSession(const uint8_t *i_pData, size_t i_szData)
{
    size_t        l_szOffset       = 0;
    stuP7baseHdr *l_pHdr           = nullptr;
    uint32_t      l_uSessionId     = uP7_PROTOCOL_SESSION_UNK;
    uint8_t       l_uSessionIdCrc7 = uP7_PROTOCOL_SESSION_CRC_UNK;
    uint32_t      l_uPacketSId     = uP7_PROTOCOL_SESSION_UNK;
    uint8_t       l_uPacketSIdCrc7 = uP7_PROTOCOL_SESSION_CRC_UNK;
    uint16_t      l_uPacketSize    = 0;
    bool          l_bResult        = false;


    if ((m_szTail + i_szData) >= FIFO_MAX_BUFFER_SIZE)
    {
        uERROR(TM("[CPU#%d] Internal buffer is too small %zu<%zu"), 
               (int)m_bId, (size_t)FIFO_MAX_BUFFER_SIZE + m_szTail, i_szData);
        return l_bResult;
    }

    if (eState::eSessionRecognition == m_cObjState)
    {
        m_qwUnknownData += i_szData;
    }

    l_szOffset = 0;
    memcpy(m_pTail + m_szTail, i_pData, i_szData);
    m_szTail += i_szData;

    while ((m_szTail - l_szOffset) >= sizeof(stuP7baseHdr))
    {
        l_pHdr = (stuP7baseHdr *)(m_pTail + l_szOffset);

        l_uPacketSId     = m_bConvertEndianess ? ntohl(l_pHdr->uSessionId) : l_pHdr->uSessionId;
        l_uPacketSIdCrc7 = l_pHdr->bExt1Crc7 & uP7_CRC7_MASK;

        const stPreProcessorFile *l_pDesc = FindDescription(l_uPacketSId);
        if (    (l_pDesc)
             && (l_pDesc->uSessionCrc7 == l_uPacketSIdCrc7)
           )
        {
            if (eState::eSessionRecognition == m_cObjState)
            {
                l_uSessionId     = l_uPacketSId;  
                l_uSessionIdCrc7 = l_uPacketSIdCrc7;
                l_bResult        = true;
            }
            else
            {
                if (l_uPacketSId == m_uSessionId)
                {
                    l_uSessionId     = l_uPacketSId;  
                    l_uSessionIdCrc7 = l_uPacketSIdCrc7;
                    l_bResult        = true;
                }
            }
        }

        if (l_bResult)
        {
            size_t l_uProcessed = l_szOffset;

            //process data
            while ((m_szTail - l_uProcessed) >= sizeof(stuP7baseHdr))
            {
                l_pHdr = (stuP7baseHdr*)(m_pTail + l_uProcessed);

                l_uPacketSIdCrc7 = l_pHdr->bExt1Crc7 & uP7_CRC7_MASK;

                if (!m_bConvertEndianess)
                {
                    l_uPacketSId  = l_pHdr->uSessionId;
                    l_uPacketSize = l_pHdr->wSize;
                }
                else
                {
                    l_uPacketSId  = ntohl(l_pHdr->uSessionId);
                    l_uPacketSize = ntohs(l_pHdr->wSize);
                }

                if (l_uSessionId == l_uPacketSId)
                {
                    if ((m_szTail - l_uProcessed) >= l_uPacketSize)
                    {
                        uint64_t l_qwMask = 1ull << l_pHdr->bType;
                        for (size_t l_szI = 0; l_szI < uP7_CPU_STREAMS_COUNT; l_szI++)
                        {
                            if (l_qwMask & m_cStreams[l_szI].qwPacketsMap)
                            {
                                AddPacket(m_cStreams[l_szI], l_pHdr);
                            }
                        }

                        l_uProcessed  += l_uPacketSize;
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    ReleaseStreamsPackets();
                    l_bResult = false;
                    break;
                }
            }//while ((m_szTail-l_uProcessed) >= sizeof(stuP7baseHdr))

            if (l_bResult)
            {
                l_szOffset = l_uProcessed;
                break;
            }
        }

        l_szOffset++;
    }



    if (l_bResult)
    {
        if (eState::eSessionRecognition == m_cObjState)
        {
            l_bResult = ApplyDescription(l_uSessionId);
        }

        if (l_bResult)
        {
            m_uSessionId = l_uSessionId;
            m_uCrc7      = l_uSessionIdCrc7;
            m_cObjState = eState::eProcessing;

            //process data
            PROCESS_STREAMS_DATA();

            m_qwUnknownData = 0ull;
        }
    }

    ReleaseStreamsPackets();

    //Get tail remainder size
    if (l_szOffset) 
    {
        m_szTailRest = 0;
        m_szTail    -= l_szOffset; 

        if (m_szTail)              
        {
            memmove(m_pTail, m_pTail + l_szOffset, m_szTail);
            l_szOffset = 0;

            if (m_szTail >= sizeof(stuP7baseHdr))
            {
                m_szTailRest = (m_bConvertEndianess ? ntohs(m_pTailHdr->wSize) : (size_t)m_pTailHdr->wSize) - m_szTail;
            }
            else
            {
                m_szTailRest = sizeof(stuP7baseHdr) - m_szTail;
            }
        }
    }


    if (!l_bResult)
    {
        if (    (eState::eSessionRecognition == m_cObjState)
             && (CPU_MAX_SESSION_UNK_SIZE <= m_qwUnknownData)
           )
        {
            uERROR(TM("[CPU#%d] Can't recognize the session ID, max attempts is reached"), (int)m_bId); 
            uERROR(TM("[CPU#%d] Perhaps session file (*.up7) is missing?"), (int)m_bId); 
            m_cObjState = eState::eError;
        }
        else
        {
            uWARNING(TM("[CPU#%d] Can't find session ID in stream, drop %zu bytes"), (int)m_bId, i_szData); 
        }
    }

    return l_bResult;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const stPreProcessorFile *CProxyCpu::FindDescription(uint32_t i_uSessionId)
{
    stPreProcessorFile *l_pFile = m_pFiles->Find(i_uSessionId);
    if (l_pFile)
    {
        if (!l_pFile->pData)
        {
            CPFile l_cFile;
            if (l_cFile.Open(l_pFile->pFileName->Get(), IFile::EOPEN | IFile::EACCESS_READ | IFile::ESHARE_READ))
            {
                tUINT64 l_qwSize = l_cFile.Get_Size();
                tUINT8 *l_pData  = (tUINT8 *)malloc((size_t)l_qwSize);

                if (l_pData)
                {
                    if (l_qwSize == (tUINT64)l_cFile.Read(l_pData, (size_t)l_qwSize))
                    {
                        l_pFile->pData  = l_pData;
                        l_pFile->szData = (size_t)l_qwSize;
                        l_pData         = NULL;
                    }
                    else
                    {
                        uERROR(TM("[CPU#%d] memory reading %I64d failed"), (int)m_bId, l_qwSize); 
                        l_pFile = NULL;
                    }
                }
                else
                {
                    uERROR(TM("[CPU#%d] memory allocation %I64d failed"), (int)m_bId, l_qwSize); 
                    l_pFile = NULL;
                }

                if (l_pData)
                {
                    free(l_pData);
                    l_pData = NULL;
                }

                l_cFile.Close(FALSE);
            }
            else
            {
                uERROR(TM("[CPU#%d] File {%s} can't be opened"), (int)m_bId, l_pFile->pFileName->Get()); 
                l_pFile = NULL;
            }
        }
    }
    else
    {
        //uERROR(TM("[CPU#%d] File for session ID#%08X not found"), (int)m_bId, i_uSessionId); 
        l_pFile = NULL;
    }

    return l_pFile;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CProxyCpu::Maintain()
{
    bool l_bReturn = true;
    for (size_t l_szI = 0; l_szI < uP7_CPU_STREAMS_COUNT; l_szI++)
    {
        l_bReturn &= m_cStreams[l_szI].iStream->Maintain();
    }

    return l_bReturn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CProxyCpu::AddBlockPackets()
{
    m_pBlockPackets = new stBlock(m_pBlockPackets);
    for (size_t l_szI = 0; l_szI < m_uBlockSize; l_szI++)
    {
        m_pBlockPackets->pPackets[l_szI].pNext = m_pFreePackets;
        m_pFreePackets = &m_pBlockPackets->pPackets[l_szI];
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CProxyCpu::AddPacket(stStream &i_rStream, stuP7baseHdr *i_pHdr)
{
    stProxyPacket *l_pNewPacket = m_pFreePackets;

    if (!l_pNewPacket)
    {
        AddBlockPackets();
        l_pNewPacket = m_pFreePackets;
    }

    m_pFreePackets      = m_pFreePackets->pNext;
    l_pNewPacket->pHdr  = i_pHdr;
    l_pNewPacket->pNext = NULL;
    
    if (!i_rStream.pLast)
    {
        i_rStream.pLast  = l_pNewPacket;
        i_rStream.pFirst = l_pNewPacket;
        l_pNewPacket->pNext = NULL;
    }
    else
    {
        i_rStream.pLast->pNext = l_pNewPacket;
        i_rStream.pLast        = l_pNewPacket;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CProxyCpu::ApplyDescription(uint32_t i_uSessionId)
{
    const stPreProcessorFile *l_pDesc = FindDescription(i_uSessionId);
    bool  l_bReturn = false;
    
    if (    (l_pDesc)
         && (l_pDesc->pData)
       )
    {
        l_bReturn = true;
        for (size_t l_szI = 0; l_szI < uP7_CPU_STREAMS_COUNT; l_szI++)
        {
            l_bReturn &= m_cStreams[l_szI].iStream->ParseDescription(i_uSessionId, l_pDesc->uSessionCrc7, l_pDesc->pData, l_pDesc->szData);
        }
    
        if (l_bReturn)
        {
            stuP7timeReqHdr l_stTimeRequest;
            l_stTimeRequest.stBase.bType      = uP7packetCreateTimeRequest;
            l_stTimeRequest.stBase.uSessionId = i_uSessionId;
            l_stTimeRequest.stBase.bExt1Crc7  = l_pDesc->uSessionCrc7;
            l_stTimeRequest.stBase.wSize      = sizeof(stuP7baseHdr);
    
            if (m_bConvertEndianess)
            {
                l_stTimeRequest.stBase.uSessionId = ntohl(l_stTimeRequest.stBase.uSessionId);
                l_stTimeRequest.stBase.wSize      = ntohs(l_stTimeRequest.stBase.wSize);
            }

            for (size_t l_szI = 0; l_szI < uP7_CPU_STREAMS_COUNT; l_szI++)
            {
                m_cStreams[l_szI].iStream->UpdateStartTime(m_cStreams[l_szI].pFirst);
            }

            if (m_iFifo->Send(&l_stTimeRequest, sizeof(stuP7baseHdr)))
            {
                for (size_t l_szI = 0; l_szI < uP7_CPU_STREAMS_COUNT; l_szI++)
                {
                    m_cStreams[l_szI].iStream->WaitTimeResponseAsync();
                    //N.B.: Start will be called from receiving time sync message or by timeout
                }
            }
            else 
            {
                if (!m_iTime)
                {
                    uWARNING(TM("[CPU#%d] Can't get CPU time, session start time is inaccurate"), (int)m_bId);
                }

                for (size_t l_szI = 0; l_szI < uP7_CPU_STREAMS_COUNT; l_szI++)
                {
                    m_cStreams[l_szI].iStream->Start();
                }
            }
        }
    }

    return l_bReturn;
}
    
    

