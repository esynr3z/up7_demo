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
CProxyTelemetry::CProxyTelemetry(CWString      &i_rName, 
                                 bool           i_bConvertEndianess,
                                 CuP7Fifo      *i_iFifo,
                                 uint8_t        i_bId,
                                 uint64_t       i_qwFreq,
                                 IuP7Time      *i_iTime, 
                                 IP7_Trace     *i_pP7Trace,
                                 IP7_Telemetry *i_pP7Tel,
                                 IProxyClient  *i_pClient
                                )
    : CProxyStream(i_rName, 
                   i_bConvertEndianess,
                   i_iFifo,
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
    for (size_t l_szI = 0; l_szI < 256; l_szI++)
    {
        m_cDesc.Add_After(m_cDesc.Get_Last(), NULL);
    }

    m_cDesc[0]; //build index

    INIT_EXT_HEADER(m_sHeader_Info.sCommonRaw, EP7USER_TYPE_TELEMETRY_V2, EP7TEL_TYPE_INFO, sizeof(sP7Tel_Info));
    //m_sHeader_Info.sCommon.dwSize    = sizeof(sP7Trace_Info);
    //m_sHeader_Info.sCommon.dwType    = EP7USER_TYPE_TELEMETRY;
    //m_sHeader_Info.sCommon.dwSubType = EP7TEL_TYPE_INFO;

    PUStrCpy(m_sHeader_Info.pName, P7TELEMETRY_NAME_LENGTH, m_cName.Get());
    
    GetEpochTime(&m_sHeader_Info.dwTime_Hi, &m_sHeader_Info.dwTime_Lo);
    m_sHeader_Info.qwTimer_Value     = m_iTime ? m_iTime->GetTime() : 0;   //CPU start timestamp
    m_sHeader_Info.qwTimer_Frequency = m_iTime ? m_iTime->GetFrequency() : i_qwFreq;
    m_sHeader_Info.qwFlags           = P7TELEMETRY_FLAG_TIME_ZONE | P7TELEMETRY_FLAG_EXTENTION;

    AddChunk(&m_sHeader_Info, sizeof(m_sHeader_Info));

    INIT_EXT_HEADER(m_sHeader_Utc.sCommonRaw, EP7USER_TYPE_TELEMETRY_V2, EP7TEL_TYPE_UTC_OFFS, sizeof(sP7Tel_Utc_Offs_V2));
    m_sHeader_Utc.iUtcOffsetSec = GetUtcOffsetSeconds();

    AddChunk(&m_sHeader_Utc, sizeof(m_sHeader_Utc));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CProxyTelemetry::~CProxyTelemetry()
{
    m_cDesc.Clear(TRUE);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CProxyTelemetry::ParseDescription(uint32_t i_uSessionId, uint8_t i_uCrc7, const uint8_t *i_pDesc, size_t i_szDesc)
{
    if (    (eStateDescription != m_eState)
         || (!i_pDesc)
         || (!i_szDesc)
       )
    {
        return false;
    }

    LOCK_ENTER(m_hLock);

    bool l_bReturn = true;

    m_uSessionId = i_uSessionId;
    m_uCrc7      = i_uCrc7;

    i_pDesc  += sizeof(stuP7SessionFileHeader);
    i_szDesc -= sizeof(stuP7SessionFileHeader);

    while (i_szDesc)
    {
        sP7Ext_Header *l_pHdr = (sP7Ext_Header*)i_pDesc;
        if (    (EP7USER_TYPE_TELEMETRY_V2 == l_pHdr->dwType)
             && (EP7TEL_TYPE_COUNTER       == l_pHdr->dwSubType)
           )
        {
            sP7Tel_Counter_v2 *l_pFormat = (sP7Tel_Counter_v2 *)i_pDesc;
            //fill by empty elements if ID is larger than elements count
            while (m_cDesc.Count() <= l_pFormat->wID)
            {
                m_cDesc.Add_After(m_cDesc.Get_Last(), NULL);
            }

            if (NULL == m_cDesc[l_pFormat->wID])
            {
                m_cDesc.Put_Data(m_cDesc.Get_ByIndex(l_pFormat->wID), new CTelDesc(l_pFormat), TRUE);
            }
        }

        if (i_szDesc >= l_pHdr->dwSize)
        {
            i_pDesc  += l_pHdr->dwSize;
            i_szDesc -= l_pHdr->dwSize;
        }
        else
        {
            uCRITICAL(TM("Parsing telemetry description error!"), 0);
            l_bReturn = false;
            break;
        }
    }


    if (l_bReturn)
    {
        AddCounters();

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
void CProxyTelemetry::UpdateStartTime(const stProxyPacket *i_pPackets)
{
    LOCK_ENTER(m_hLock);

    GetEpochTime(&m_sHeader_Info.dwTime_Hi, &m_sHeader_Info.dwTime_Lo);
    m_qwHostProxyCreationTime = GetPerformanceCounter();

    bool l_bFound = false;

    while (    (i_pPackets)    
            && (!l_bFound)
          )
    {
        if (uP7packetTelemetryDouble == i_pPackets->pHdr->bType)
        {
            stuP7telF8Hdr *l_pHdrIn = (stuP7telF8Hdr *)i_pPackets->pHdr;

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
        else if (uP7packetTelemetryFloat == i_pPackets->pHdr->bType)
        {
            stuP7telF4Hdr *l_pHdrIn = (stuP7telF4Hdr *)i_pPackets->pHdr;

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
        else if (uP7packetTelemetryInt64 == i_pPackets->pHdr->bType)
        {
            stuP7telI8Hdr *l_pHdrIn  = (stuP7telI8Hdr *)i_pPackets->pHdr;

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
        else if (uP7packetTelemetryInt32 == i_pPackets->pHdr->bType)
        {
            stuP7telI4Hdr *l_pHdrIn  = (stuP7telI4Hdr *)i_pPackets->pHdr;

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
        else if (uP7packetCreateTimeResponse == i_pPackets->pHdr->bType)
        {
            stuP7timeResHdr *l_pHdrIn  = (stuP7timeResHdr *)i_pPackets->pHdr;

            if (!m_bConvertEndianess)
            {
                m_qwCpuStartTime = l_pHdrIn->qwCpuCurrenTime;
            }
            else
            {
                m_qwCpuStartTime = ntohqw(l_pHdrIn->qwCpuCurrenTime);
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
void CProxyTelemetry::Start()
{
    LOCK_ENTER(m_hLock);

    if (m_bTimeInSync)
    {
        CProxyStream::AllignSessionTime<sP7Tel_Info*>(&m_sHeader_Info);
    }

    m_eState = eStateReady;
    SendChunks();

    LOCK_EXIT(m_hLock);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CProxyTelemetry::AddCounters()
{
    pAList_Cell l_pEl = NULL;
    while ((l_pEl = m_cDesc.Get_Next(l_pEl)))
    {
        CTelDesc *l_pCnt = m_cDesc.Get_Data(l_pEl);
        if (l_pCnt)
        {
            sP7Tel_Counter_v2 *l_pP7Cnt = l_pCnt->GetData();
            AddChunk(l_pP7Cnt, l_pP7Cnt->sCommon.dwSize);
        }
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
size_t CProxyTelemetry::Process(const stProxyPacket *i_pPackets)
{
    LOCK_ENTER(m_hLock);

    if (eStateError != m_eState)
    {
        while (i_pPackets)
        {
            if (uP7packetTelemetryDouble == i_pPackets->pHdr->bType)
            {
                stuP7baseHdr    *l_pHdrIn  = (stuP7baseHdr *)i_pPackets->pHdr;
                sP7Tel_Value_v2 *l_pHdrOut = (sP7Tel_Value_v2*)m_cPool.Use(sizeof(sP7Tel_Value_v2));

                if (l_pHdrOut)
                {
                    INIT_EXT_HEADER(l_pHdrOut->sCommonRaw, EP7USER_TYPE_TELEMETRY_V2, EP7TEL_TYPE_VALUE, sizeof(sP7Tel_Value_v2));
                    //l_pHdrOut->sCommon.dwSize    = sizeof(sP7Tel_Value); 
                    //l_pHdrOut->sCommon.dwType    = EP7USER_TYPE_TELEMETRY;
                    //l_pHdrOut->sCommon.dwSubType = EP7TEL_TYPE_VALUE;


                    if (!m_bConvertEndianess)
                    {
                        l_pHdrOut->dbValue = ((stuP7telF8Hdr*)l_pHdrIn)->tValue;
                        l_pHdrOut->qwTimer = ((stuP7telF8Hdr*)l_pHdrIn)->qwTime;
                        l_pHdrOut->wID     = ((stuP7telF8Hdr*)l_pHdrIn)->wId;
                    }
                    else
                    {
                        l_pHdrOut->sCommonRaw.dwBits = ntohl(l_pHdrOut->sCommonRaw.dwBits);
                        l_pHdrOut->dbValue = ntohdb(((stuP7telF8Hdr*)l_pHdrIn)->tValue);
                        l_pHdrOut->qwTimer = ntohqw(((stuP7telF8Hdr*)l_pHdrIn)->qwTime);
                        l_pHdrOut->wID     = ntohs(((stuP7telF8Hdr*)l_pHdrIn)->wId);
                    }

                    l_pHdrOut->qwTimer = (uint64_t)((int64_t)l_pHdrOut->qwTimer + m_llTimeCorrection);
                }
            }
            else if (uP7packetTelemetryFloat == i_pPackets->pHdr->bType)
            {
                stuP7baseHdr    *l_pHdrIn  = (stuP7baseHdr *)i_pPackets->pHdr;
                sP7Tel_Value_v2 *l_pHdrOut = (sP7Tel_Value_v2*)m_cPool.Use(sizeof(sP7Tel_Value_v2));

                if (l_pHdrOut)
                {
                    INIT_EXT_HEADER(l_pHdrOut->sCommonRaw, EP7USER_TYPE_TELEMETRY_V2, EP7TEL_TYPE_VALUE, sizeof(sP7Tel_Value_v2));
                    //l_pHdrOut->sCommon.dwSize    = sizeof(sP7Tel_Value); 
                    //l_pHdrOut->sCommon.dwType    = EP7USER_TYPE_TELEMETRY;
                    //l_pHdrOut->sCommon.dwSubType = EP7TEL_TYPE_VALUE;

                    if (!m_bConvertEndianess)
                    {
                        l_pHdrOut->dbValue = ((stuP7telF4Hdr*)l_pHdrIn)->tValue;
                        l_pHdrOut->qwTimer = ((stuP7telF4Hdr*)l_pHdrIn)->qwTime;
                        l_pHdrOut->wID     = ((stuP7telF4Hdr*)l_pHdrIn)->wId;
                    }
                    else
                    {
                        l_pHdrOut->sCommonRaw.dwBits = ntohl(l_pHdrOut->sCommonRaw.dwBits);
                        l_pHdrOut->dbValue = (tDOUBLE)ntohft(((stuP7telF4Hdr*)l_pHdrIn)->tValue);
                        l_pHdrOut->qwTimer = ntohqw(((stuP7telF4Hdr*)l_pHdrIn)->qwTime);
                        l_pHdrOut->wID     = ntohs(((stuP7telF4Hdr*)l_pHdrIn)->wId);
                    }

                    l_pHdrOut->qwTimer = (uint64_t)((int64_t)l_pHdrOut->qwTimer + m_llTimeCorrection);
                }
            }
            else if (uP7packetTelemetryInt64 == i_pPackets->pHdr->bType)
            {
                stuP7baseHdr    *l_pHdrIn  = (stuP7baseHdr *)i_pPackets->pHdr;
                sP7Tel_Value_v2 *l_pHdrOut = (sP7Tel_Value_v2*)m_cPool.Use(sizeof(sP7Tel_Value_v2));

                if (l_pHdrOut)
                {
                    INIT_EXT_HEADER(l_pHdrOut->sCommonRaw, EP7USER_TYPE_TELEMETRY_V2, EP7TEL_TYPE_VALUE, sizeof(sP7Tel_Value_v2));
                    //l_pHdrOut->sCommon.dwSize    = sizeof(sP7Tel_Value); 
                    //l_pHdrOut->sCommon.dwType    = EP7USER_TYPE_TELEMETRY;
                    //l_pHdrOut->sCommon.dwSubType = EP7TEL_TYPE_VALUE;


                    if (!m_bConvertEndianess)
                    {
                        l_pHdrOut->dbValue = (tDOUBLE)((stuP7telI8Hdr*)l_pHdrIn)->tValue;
                        l_pHdrOut->qwTimer = ((stuP7telI8Hdr*)l_pHdrIn)->qwTime;
                        l_pHdrOut->wID     = ((stuP7telI8Hdr*)l_pHdrIn)->wId;
                    }
                    else
                    {
                        l_pHdrOut->sCommonRaw.dwBits = ntohl(l_pHdrOut->sCommonRaw.dwBits);
                        l_pHdrOut->dbValue = (tDOUBLE)ntohqw(((stuP7telI8Hdr*)l_pHdrIn)->tValue);
                        l_pHdrOut->qwTimer = ntohqw(((stuP7telI8Hdr*)l_pHdrIn)->qwTime);
                        l_pHdrOut->wID     = ntohs(((stuP7telI8Hdr*)l_pHdrIn)->wId);
                    }

                    l_pHdrOut->qwTimer = (uint64_t)((int64_t)l_pHdrOut->qwTimer + m_llTimeCorrection);
                }
            }
            else if (uP7packetTelemetryInt32 == i_pPackets->pHdr->bType)
            {
                stuP7baseHdr    *l_pHdrIn  = (stuP7baseHdr *)i_pPackets->pHdr;
                sP7Tel_Value_v2 *l_pHdrOut = (sP7Tel_Value_v2*)m_cPool.Use(sizeof(sP7Tel_Value_v2));

                if (l_pHdrOut)
                {
                    INIT_EXT_HEADER(l_pHdrOut->sCommonRaw, EP7USER_TYPE_TELEMETRY_V2, EP7TEL_TYPE_VALUE, sizeof(sP7Tel_Value_v2));
                    //l_pHdrOut->sCommon.dwSize    = sizeof(sP7Tel_Value); 
                    //l_pHdrOut->sCommon.dwType    = EP7USER_TYPE_TELEMETRY;
                    //l_pHdrOut->sCommon.dwSubType = EP7TEL_TYPE_VALUE;

                    if (!m_bConvertEndianess)
                    {
                        l_pHdrOut->dbValue = ((stuP7telI4Hdr*)l_pHdrIn)->tValue;
                        l_pHdrOut->qwTimer = ((stuP7telI4Hdr*)l_pHdrIn)->qwTime;
                        l_pHdrOut->wID     = ((stuP7telI4Hdr*)l_pHdrIn)->wId;
                    }
                    else
                    {
                        l_pHdrOut->sCommonRaw.dwBits = ntohl(l_pHdrOut->sCommonRaw.dwBits);
                        l_pHdrOut->dbValue = (tDOUBLE)ntohl(((stuP7telI4Hdr*)l_pHdrIn)->tValue);
                        l_pHdrOut->qwTimer = ntohqw(((stuP7telI4Hdr*)l_pHdrIn)->qwTime);
                        l_pHdrOut->wID     = ntohs(((stuP7telI4Hdr*)l_pHdrIn)->wId);
                    }

                    l_pHdrOut->qwTimer = (uint64_t)((int64_t)l_pHdrOut->qwTimer + m_llTimeCorrection);
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
                
                CProxyStream::AllignSessionTime<sP7Tel_Info*>(&m_sHeader_Info);
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
uint64_t CProxyTelemetry::GetSupportedPacketsMap()
{ 
    return    (1ull << uP7packetTelemetryDouble)
            | (1ull << uP7packetTelemetryFloat)
            | (1ull << uP7packetTelemetryInt64)
            | (1ull << uP7packetTelemetryInt32)
            | (1ull << uP7packetTelemetryOnOff)
            | (1ull << uP7packetCreateTimeResponse);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CProxyTelemetry::On_Receive(tUINT32 i_dwChannel, 
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

            if (EP7USER_TYPE_TELEMETRY_V2 == GET_EXT_HEADER_TYPE(l_sHeader))
            {
                if (EP7TEL_TYPE_ENABLE == GET_EXT_HEADER_SUBTYPE(l_sHeader))
                {
                    sP7Tel_Enable_v2 *l_pEnableIn = (sP7Tel_Enable_v2*)i_pBuffer;

                    stuP7telOnOffHdr l_stEnableOut = {};

                    l_stEnableOut.stBase.bType     = uP7packetTelemetryOnOff;
                    l_stEnableOut.stBase.bExt1Crc7 = m_uCrc7;
                    l_stEnableOut.bOn              = (uint8_t)l_pEnableIn->bOn;

                    if (m_bConvertEndianess)
                    {
                        l_stEnableOut.stBase.uSessionId = htonl(m_uSessionId);
                        l_stEnableOut.stBase.wSize      = htons((uint16_t)sizeof(stuP7telOnOffHdr));
                        l_stEnableOut.wId               = htons(l_pEnableIn->wID);
                    }
                    else
                    {
                        l_stEnableOut.stBase.uSessionId = m_uSessionId;
                        l_stEnableOut.stBase.wSize      = sizeof(stuP7telOnOffHdr);
                        l_stEnableOut.wId               = l_pEnableIn->wID;
                    }

                    if (i_bBigEndian)
                    {
                        l_stEnableOut.wId = htons(l_stEnableOut.wId);
                    }
    
                    if (!m_pFifo->Send(&l_stEnableOut, sizeof(stuP7telOnOffHdr)))
                    {
                        uWARNING(TM("[CPU#%d] Can't send verobsity update"), (int)m_bId);
                    }
                }
                else if (EP7TEL_TYPE_DELETE == GET_EXT_HEADER_SUBTYPE(l_sHeader))
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
void CProxyTelemetry::On_Status(tUINT32 i_dwChannel, const sP7C_Status *i_pStatus)
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
            AddCounters();
        }

        m_stStatus = *i_pStatus;
    }
    LOCK_EXIT(m_hLock);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CProxyTelemetry::On_Flush(tUINT32 i_dwChannel, tBOOL *io_pCrash)
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

