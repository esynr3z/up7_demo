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

#define P7_TRC_MODULE_EXT_SIZE 4

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CProxyTrace::CProxyTrace(CWString      &i_rName, 
                         bool           i_bConvertEndianess,
                         CuP7Fifo      *i_pFifo,
                         uint8_t        i_bId,
                         uint64_t       i_qwFreq,
                         IuP7Time      *i_iTime, 
                         IP7_Trace     *i_pP7Trace,
                         IP7_Telemetry *i_pP7Tel,
                         IProxyClient  *i_pClient
                        )
    : CProxyStream(i_rName, 
                   i_bConvertEndianess,
                   i_pFifo,
                   i_bId,
                   i_qwFreq,
                   i_iTime,  
                   i_pP7Trace,
                   i_pP7Tel,
                   i_pClient)
{
    if (m_pP7Trace)
    {
        m_pP7Trace->Register_Module(TM("ProxyTrc"), &m_hP7Mod);
    }

    //preallocate elements
    for (size_t l_szI = 0; l_szI < 8192; l_szI++)
    {
        m_cTrcDesc.Add_After(m_cTrcDesc.Get_Last(), NULL);
    }

    for (size_t l_szI = 0; l_szI < 64; l_szI++)
    {
        m_cModDesc.Add_After(m_cModDesc.Get_Last(), NULL);
    }

    m_cTrcDesc[0]; //build index
    m_cModDesc[0];

    INIT_EXT_HEADER(m_sHeader_Info.sCommonRaw, EP7USER_TYPE_TRACE, EP7TRACE_TYPE_INFO, sizeof(m_sHeader_Info));
    //m_sHeader_Info.sCommon.dwSize    = sizeof(sP7Trace_Info);
    //m_sHeader_Info.sCommon.dwType    = EP7USER_TYPE_TRACE;
    //m_sHeader_Info.sCommon.dwSubType = EP7TRACE_TYPE_INFO;
    PUStrCpy(m_sHeader_Info.pName, P7TRACE_NAME_LENGTH, m_cName.Get());

    GetEpochTime(&m_sHeader_Info.dwTime_Hi, &m_sHeader_Info.dwTime_Lo);
    m_sHeader_Info.qwTimer_Value     = m_iTime ? m_iTime->GetTime() : 0;  //CPU start timestamp
    m_sHeader_Info.qwTimer_Frequency = m_iTime ? m_iTime->GetFrequency() : i_qwFreq;
    m_sHeader_Info.qwFlags           = P7TRACE_INFO_FLAG_EXTENTION | P7TRACE_INFO_FLAG_TIME_ZONE;

    AddChunk(&m_sHeader_Info, sizeof(m_sHeader_Info));

    //Add utc offset information
    INIT_EXT_HEADER(m_sHeader_Utc.sCommonRaw, EP7USER_TYPE_TRACE, EP7TRACE_TYPE_UTC_OFFS, sizeof(m_sHeader_Utc));
    m_sHeader_Utc.iUtcOffsetSec = GetUtcOffsetSeconds();

    AddChunk(&m_sHeader_Utc, sizeof(m_sHeader_Utc));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CProxyTrace::~CProxyTrace()
{
    m_cTrcDesc.Clear(TRUE);
    m_cModDesc.Clear(TRUE);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CProxyTrace::ParseDescription(uint32_t i_uSessionId, uint8_t i_uCrc7, const uint8_t *i_pDesc, size_t i_szDesc)
{
    if (    (eStateDescription != m_eState)
         || (!i_pDesc)
         || (!i_szDesc)
       )
    {
        return false;
    }

    LOCK_ENTER(m_hLock);

    m_uSessionId = i_uSessionId;
    m_uCrc7      = i_uCrc7;
    i_pDesc  += sizeof(stuP7SessionFileHeader);
    i_szDesc -= sizeof(stuP7SessionFileHeader);

    bool l_bReturn = true;

    while (i_szDesc)
    {
        sP7Ext_Header *l_pHdr = (sP7Ext_Header*)i_pDesc;
        if (EP7USER_TYPE_TRACE == l_pHdr->dwType)
        {
            if (EP7TRACE_TYPE_DESC == l_pHdr->dwSubType)
            {
                sP7Trace_Format *l_pFormat = (sP7Trace_Format *)i_pDesc;
                //fill by empty elements if ID is larger than elements count
                while (m_cTrcDesc.Count() <= l_pFormat->wID)
                {
                    m_cTrcDesc.Add_After(m_cTrcDesc.Get_Last(), NULL);
                }

                if (NULL == m_cTrcDesc[l_pFormat->wID])
                {
                    m_cTrcDesc.Put_Data(m_cTrcDesc.Get_ByIndex(l_pFormat->wID), new CTraceDesc(l_pFormat), TRUE);
                }
            }
            else if (EP7TRACE_TYPE_MODULE == l_pHdr->dwSubType)
            {
                sP7Trace_Module *l_pModule = (sP7Trace_Module *)i_pDesc;
                //fill by empty elements if ID is larger than elements count
                while (m_cModDesc.Count() <= l_pModule->wModuleID)
                {
                    m_cModDesc.Add_After(m_cModDesc.Get_Last(), NULL);
                }

                if (NULL == m_cModDesc[l_pModule->wModuleID])
                {
                    m_cModDesc.Put_Data(m_cModDesc.Get_ByIndex(l_pModule->wModuleID), new CModuleDesc(l_pModule), TRUE);
                }
            }
        }

        if (i_szDesc >= l_pHdr->dwSize)
        {
            i_pDesc  += l_pHdr->dwSize;
            i_szDesc -= l_pHdr->dwSize;
        }
        else
        {
            uCRITICAL(TM("[CPU#%d] Parsing trace description error!"), (int)m_bId);
            l_bReturn = false;
            break;
        }
    }

    if (l_bReturn)
    {
        AddModules();

        m_pClient = m_pProxyClient->RegisterChannel(this);
        if (m_pClient)
        {
            m_eState = eStateReady;
        }
        else
        {
            uCRITICAL(TM("[CPU#%d] m_pProxyClient->RegisterChannel() failed"), (int)m_bId);
            m_eState = eStateError;
            l_bReturn = false;
        }
    }
    else
    {
        m_eState = eStateError;
    }

    LOCK_EXIT(m_hLock);


    return l_bReturn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CProxyTrace::UpdateStartTime(const stProxyPacket *i_pPackets)
{
    LOCK_ENTER(m_hLock);

    GetEpochTime(&m_sHeader_Info.dwTime_Hi, &m_sHeader_Info.dwTime_Lo);
    m_qwHostProxyCreationTime = GetPerformanceCounter();

    bool l_bFound = false;

    while (    (i_pPackets)    
            && (!l_bFound)
          )
    {
        if (uP7packetTrace == i_pPackets->pHdr->bType)
        {
            stuP7traceHdr *l_pHdrIn = (stuP7traceHdr *)i_pPackets->pHdr;

            if (!m_bConvertEndianess)
            {
                m_qwCpuStartTime = l_pHdrIn->qwTime;
            }
            else
            {
                m_qwCpuStartTime = ntohqw(l_pHdrIn->qwTime);
            }

            l_bFound = true;
        }

        i_pPackets = i_pPackets->pNext;
    }


    if (m_iTime)
    {
        m_qwCpuProxyCreationTime = m_iTime->GetTime();
    }
    else
    {
        m_qwCpuProxyCreationTime = m_qwCpuStartTime;
    }

    m_sHeader_Info.qwTimer_Value = m_qwCpuStartTime;

    if (!l_bFound)
    {
        uERROR(TM("[CPU#%d] ERROR: can't extract first packet time stamp"), (int)m_bId);
    }

    LOCK_EXIT(m_hLock);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CProxyTrace::Start()
{
    LOCK_ENTER(m_hLock);

    if (m_bTimeInSync)
    {
        CProxyStream::AllignSessionTime<sP7Trace_Info*>(&m_sHeader_Info);
    }

    m_eState = eStateReady;
    SendChunks();

    LOCK_EXIT(m_hLock);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CProxyTrace::AddModules()
{
    pAList_Cell l_pEl = NULL;
    while ((l_pEl = m_cModDesc.Get_Next(l_pEl)))
    {
        CModuleDesc *l_pMod = m_cModDesc.Get_Data(l_pEl);
        if (l_pMod)
        {
            sP7Trace_Module *l_pP7Mod = l_pMod->GetData();
            AddChunk(l_pP7Mod, l_pP7Mod->sCommon.dwSize);
        }
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
size_t CProxyTrace::Process(const stProxyPacket *i_pPackets)
{
    LOCK_ENTER(m_hLock);

    if (eStateError != m_eState)
    {
        while (i_pPackets)
        {
            if (uP7packetTrace == i_pPackets->pHdr->bType)
            {
                stuP7traceHdr *l_pHdrIn       = (stuP7traceHdr *)i_pPackets->pHdr;
                size_t         l_szHdrOutBase = l_pHdrIn->stBase.wSize - sizeof(stuP7traceHdr) + sizeof(sP7Trace_Data);
                sP7Trace_Data *l_pHdrOut      = (sP7Trace_Data*)m_cPool.Use(l_szHdrOutBase + P7_TRC_MODULE_EXT_SIZE);
                uint8_t       *l_pExt         = (uint8_t*)l_pHdrOut + l_szHdrOutBase;

                if (l_pHdrOut)
                {
                    CTraceDesc *l_pDesc = NULL;

                    INIT_EXT_HEADER(l_pHdrOut->sCommonRaw, EP7USER_TYPE_TRACE, EP7TRACE_TYPE_DATA, l_szHdrOutBase + P7_TRC_MODULE_EXT_SIZE);

                    //copy variable arguments
                    memcpy((uint8_t*)l_pHdrOut + sizeof(sP7Trace_Data),
                           (uint8_t*)l_pHdrIn + sizeof(stuP7traceHdr),
                           l_pHdrIn->stBase.wSize - sizeof(stuP7traceHdr)
                          );

                    if (!m_bConvertEndianess)
                    {
                        l_pHdrOut->bLevel     = up7_GET_TRC_LVL(l_pHdrIn->bMod13Lvl3);
                        l_pHdrOut->bProcessor = m_bId;
                        l_pHdrOut->dwSequence = l_pHdrIn->uSeqN;
                        l_pHdrOut->dwThreadID = l_pHdrIn->uThreadId;
                        l_pHdrOut->qwTimer    = l_pHdrIn->qwTime;
                        l_pHdrOut->wID        = l_pHdrIn->wId;

                        *((uint16_t*)l_pExt) = up7_GET_TRC_MOD(l_pHdrIn->bMod13Lvl3);
                        l_pExt += 2;
                        *l_pExt = (uint8_t)EP7TRACE_EXT_MODULE_ID;
                        l_pExt ++;
                        *l_pExt = 1; //end of extension

                        l_pDesc = m_cTrcDesc[l_pHdrOut->wID];
                    }
                    else
                    {
                        uint16_t wMod13Lvl3 = ntohs(l_pHdrIn->bMod13Lvl3);

                        l_pHdrOut->sCommonRaw.dwBits = ntohl(l_pHdrOut->sCommonRaw.dwBits);

                        l_pHdrOut->bLevel     = up7_GET_TRC_LVL(wMod13Lvl3);
                        l_pHdrOut->bProcessor = m_bId;
                        l_pHdrOut->dwSequence = ntohl(l_pHdrIn->uSeqN);
                        l_pHdrOut->dwThreadID = ntohl(l_pHdrIn->uThreadId);
                        l_pHdrOut->qwTimer    = ntohqw(l_pHdrIn->qwTime);
                        l_pHdrOut->wID        = ntohs(l_pHdrIn->wId);

                        *((uint16_t*)l_pExt) = up7_GET_TRC_MOD(wMod13Lvl3);
                        l_pExt += 2;
                        *l_pExt = (uint8_t)EP7TRACE_EXT_MODULE_ID;
                        l_pExt ++;
                        *l_pExt = 1; //end of extension

                        l_pDesc = m_cTrcDesc[l_pHdrOut->wID];

                        //Converting arguments to another endian
                        uint8_t         *l_pVArgs  = (uint8_t*)l_pHdrOut + sizeof(sP7Trace_Data);
                        sP7Trace_Format *l_pFormat = l_pDesc->GetData();
                        sP7Trace_Arg    *l_pArgs   = (sP7Trace_Arg*)((uint8_t*)l_pFormat + sizeof(sP7Trace_Format));

                        for (uint32_t l_uI = 0; l_uI < l_pFormat->wArgs_Len; l_uI++)
                        {
                            tUINT8 l_bSize = l_pArgs[l_uI].bSize;
                            if (4 == l_bSize)
                            {
                                *(uint32_t*)l_pVArgs = ntohl(*(uint32_t*)l_pVArgs);
                                l_pVArgs += 4;
                            }
                            else if (8 == l_bSize)
                            {
                                *(uint64_t*)l_pVArgs = ntohqw(*(uint64_t*)l_pVArgs);
                                l_pVArgs += 8;
                            }
                            else if (    (!l_bSize)
                                      || (uP7_STRING_UNSUPPORTED_SIZE == l_bSize)
                                    )
                            {
                                tUINT8 l_bType = l_pArgs[l_uI].bType;
                                if (P7TRACE_ARG_TYPE_USTR16 == l_bType)
                                {
                                    uint16_t *l_pStr = (uint16_t *)l_pVArgs;
                                    while (*l_pStr)
                                    {
                                       *l_pStr = ntohs(*l_pStr);
                                        l_pStr ++;
                                    }

                                    l_pVArgs = (tUINT8*)(++l_pStr);
                                }
                                else if (    (P7TRACE_ARG_TYPE_STRA == l_bType)
                                          || (P7TRACE_ARG_TYPE_USTR8 == l_bType)
                                        )
                                {
                                    while (0 != *l_pVArgs) l_pVArgs++;
                                    l_pVArgs++;
                                }
                                else if (P7TRACE_ARG_TYPE_USTR32 == l_bType)
                                {
                                    tUINT32 *l_pStr = (tUINT32 *)l_pVArgs;
                                    while (*l_pStr)
                                    {
                                       *l_pStr = ntohl(*l_pStr);
                                        l_pStr ++;
                                    }

                                    l_pVArgs = (tUINT8*)(++l_pStr);
                                }
                                else
                                {
                                    uERROR(TM("[CPU#%d] Unknown string type"), (int)m_bId);
                                }
                            }
                            else
                            {
                                uERROR(TM("[CPU#%d] Unknown arg type"), (int)m_bId);
                            }
                        }
                    }

                    l_pHdrOut->qwTimer = (uint64_t)((int64_t)l_pHdrOut->qwTimer + m_llTimeCorrection);

                    if (    (l_pDesc)
                         && (m_stStatus.dwResets != l_pDesc->GetResets())
                       )
                    {
                        sP7Trace_Format *l_pFormat = l_pDesc->GetData();
                        AddChunk(l_pFormat, l_pFormat->sCommon.dwSize);
                        l_pDesc->SetResets(m_stStatus.dwResets);
                    }
                }
            }
            else if (uP7packetTraceVerbosiry == i_pPackets->pHdr->bType)
            {
                stuP7traceVerbosityHdr *l_pHdrIn  = (stuP7traceVerbosityHdr *)i_pPackets->pHdr;
                sP7Trace_Verb          *l_pHdrOut = (sP7Trace_Verb*)m_cPool.Use(sizeof(sP7Trace_Verb));

                if (l_pHdrOut)
                {
                    INIT_EXT_HEADER(l_pHdrOut->sCommonRaw, EP7USER_TYPE_TRACE, EP7TRACE_TYPE_VERB, sizeof(sP7Trace_Verb));
                    //l_sCmd.sCommon.dwSize    = sizeof(sP7Trace_Verb);
                    //l_sCmd.sCommon.dwType    = EP7USER_TYPE_TRACE;
                    //l_sCmd.sCommon.dwSubType = EP7TRACE_TYPE_VERB;

                    l_pHdrOut->eVerbosity = (eP7Trace_Level)l_pHdrIn->bLevel;

                    if (!m_bConvertEndianess)
                    {
                        l_pHdrOut->wModuleID  = l_pHdrIn->wModuleId;
                    }
                    else
                    {
                        l_pHdrOut->wModuleID  = ntohs(l_pHdrIn->wModuleId);
                    }
                }
            }
            else if (uP7packetCreateTimeResponse == i_pPackets->pHdr->bType)
            {
                stuP7timeResHdr *l_pTime = (stuP7timeResHdr*)i_pPackets->pHdr;

                if (!m_bConvertEndianess)
                {
                    m_qwCpuProxyCreationTime = l_pTime->qwCpuCurrenTime;
                    m_qwCpuStartTime         = l_pTime->qwCpuStartTime;
                    m_qwCpuFreq              = l_pTime->qwCpuFreq;
                }
                else
                {
                    m_qwCpuProxyCreationTime = ntohqw(l_pTime->qwCpuCurrenTime);
                    m_qwCpuStartTime         = ntohqw(l_pTime->qwCpuStartTime);
                    m_qwCpuFreq              = ntohqw(l_pTime->qwCpuFreq);
                }

                m_bTimeInSync = true;

                m_sHeader_Info.qwTimer_Frequency = m_qwCpuFreq;
                m_sHeader_Info.qwTimer_Value     = m_qwCpuStartTime;
                
                CProxyStream::AllignSessionTime<sP7Trace_Info*>(&m_sHeader_Info);
            }
            i_pPackets = i_pPackets->pNext;
        }
    }

    if (eStateReady == m_eState)
    {
        SendChunks();
    }

    LOCK_EXIT(m_hLock);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint64_t CProxyTrace::GetSupportedPacketsMap()
{ 
    return   (1ull << uP7packetTrace) 
           | (1ull << uP7packetTraceVerbosiry) 
           | (1ull << uP7packetCreateTimeResponse);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CProxyTrace::On_Receive(tUINT32 i_dwChannel, 
                             tUINT8 *i_pBuffer, 
                             tUINT32 i_dwSize,
                             tBOOL   i_bBigEndian
                             )
{
    UNUSED_ARG(i_dwChannel);

    LOCK_ENTER(m_hLock);

    if (eStateCommunicationIsOn <= m_eState)
    {
        if (    (i_pBuffer)
             && (i_dwSize >= sizeof(sP7Ext_Header))
           )
        {
            sP7Ext_Raw l_sHeader = *(sP7Ext_Raw*)i_pBuffer;

            if (EP7USER_TYPE_TRACE == GET_EXT_HEADER_TYPE(l_sHeader))
            {
                if (EP7TRACE_TYPE_VERB == GET_EXT_HEADER_SUBTYPE(l_sHeader))
                {
                    if (i_bBigEndian)
                    {
                        ((sP7Trace_Verb*)i_pBuffer)->wModuleID  = htons(((sP7Trace_Verb*)i_pBuffer)->wModuleID);
                        ((sP7Trace_Verb*)i_pBuffer)->eVerbosity = (eP7Trace_Level)htonl(((sP7Trace_Verb*)i_pBuffer)->eVerbosity);
                    }

                    CModuleDesc *l_pMod = m_cModDesc[((sP7Trace_Verb*)i_pBuffer)->wModuleID];
                    if (l_pMod)
                    {
                        l_pMod->GetData()->eVerbosity = ((sP7Trace_Verb*)i_pBuffer)->eVerbosity;
                    }


                    stuP7traceVerbosityHdr l_stVerbosity = {};
                    l_stVerbosity.stBase.bType     = uP7packetTraceVerbosiry;
                    l_stVerbosity.stBase.bExt1Crc7 = m_uCrc7;
                    l_stVerbosity.bLevel           = (uint8_t)((sP7Trace_Verb*)i_pBuffer)->eVerbosity;

                    if (m_bConvertEndianess)
                    {
                        l_stVerbosity.stBase.uSessionId = htonl(m_uSessionId);
                        l_stVerbosity.stBase.wSize      = htons((uint16_t)sizeof(stuP7traceVerbosityHdr));
                        l_stVerbosity.wModuleId         = htons(((sP7Trace_Verb*)i_pBuffer)->wModuleID);
                    }
                    else
                    {
                        l_stVerbosity.stBase.uSessionId = m_uSessionId;
                        l_stVerbosity.stBase.wSize      = sizeof(stuP7traceVerbosityHdr);
                        l_stVerbosity.wModuleId         = ((sP7Trace_Verb*)i_pBuffer)->wModuleID;
                    }

    
                    if (!m_pFifo->Send(&l_stVerbosity, sizeof(stuP7traceVerbosityHdr)))
                    {
                        uWARNING(TM("[CPU#%d] Can't send verobsity update"), (int)m_bId);
                    }
                }
                else if (EP7TRACE_TYPE_DELETE == GET_EXT_HEADER_SUBTYPE(l_sHeader))
                {
                    On_Flush(m_uClientId, NULL);
                }
            }
        }

    }
    else
    {
        uWARNING(TM("[CPU#%d] Trying to send data to CPU when communication isn't yet established"), (int)m_bId);
    }

    LOCK_EXIT(m_hLock);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CProxyTrace::On_Status(tUINT32 i_dwChannel, const sP7C_Status *i_pStatus)
{
    UNUSED_ARG(i_dwChannel);

    LOCK_ENTER(m_hLock);
    if (i_pStatus)
    {
        if (    (i_pStatus->dwResets != m_stStatus.dwResets)
             && (i_pStatus->bConnected)
           )
        {
            ClearChunks();
            AddChunk(&m_sHeader_Info, sizeof(m_sHeader_Info));
            AddChunk(&m_sHeader_Utc, sizeof(m_sHeader_Utc));
            AddModules();
        }

        m_stStatus = *i_pStatus;
    }
    LOCK_EXIT(m_hLock);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CProxyTrace::On_Flush(tUINT32 i_dwChannel, tBOOL *io_pCrash)
{
    UNUSED_ARG(i_dwChannel);
    UNUSED_ARG(io_pCrash);

    LOCK_ENTER(m_hLock);

    if (!m_bClosed)
    {
        sP7Ext_Raw l_sHeader;
        INIT_EXT_HEADER(l_sHeader, EP7USER_TYPE_TRACE, EP7TRACE_TYPE_CLOSE, sizeof(sP7Ext_Raw));

        AddChunk(&l_sHeader, sizeof(sP7Ext_Raw));
        SendChunks();

        m_bClosed = true;
    }

    LOCK_EXIT(m_hLock);
}
