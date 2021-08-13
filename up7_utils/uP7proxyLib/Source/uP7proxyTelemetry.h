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
#ifndef UP7_TELEMETRY_TRACE_H
#define UP7_TELEMETRY_TRACE_H

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CTelDesc
{
    sP7Tel_Counter_v2 *m_pFormat;

public:
    CTelDesc(sP7Tel_Counter_v2 *i_pFormat)
        : m_pFormat(NULL)
    {
        if (i_pFormat)
        {
            m_pFormat = (sP7Tel_Counter_v2 *)malloc(i_pFormat->sCommon.dwSize);
            if (m_pFormat)
            {
                memcpy(m_pFormat, i_pFormat, i_pFormat->sCommon.dwSize);
            }
        }
    }

    virtual ~CTelDesc()
    {
        if (m_pFormat)
        {
            free(m_pFormat);
            m_pFormat = 0;
        }
    }

    sP7Tel_Counter_v2 *GetData()  { return m_pFormat; }

};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CProxyTelemetry
    : public CProxyStream
{                      
    CBList<CTelDesc*>    m_cDesc;
    sP7Tel_Info          m_sHeader_Info;
    sP7Tel_Utc_Offs_V2   m_sHeader_Utc;

public:
    CProxyTelemetry(CWString      &i_rName, 
                    bool           i_bConvertEndianess,
                    CuP7Fifo      *i_iFifo,
                    uint8_t        i_bId,
                    uint64_t       i_qwFreq,
                    IuP7Time      *i_iTime, 
                    IP7_Trace     *i_pP7Trace,
                    IP7_Telemetry *i_pP7Tel,
                    IProxyClient  *i_pClient
                   );

    virtual ~CProxyTelemetry();
    bool     ParseDescription(uint32_t i_uSessionId, uint8_t i_uCrc7, const uint8_t *i_pDesc, size_t i_szDesc);
    void     UpdateStartTime(const stProxyPacket *i_pPackets);
    void     Start();


    void     AddCounters();
    size_t   Process(const stProxyPacket *i_pPackets);
    uint64_t GetSupportedPacketsMap();

    //----------------------------------------------IP7C_Channel--------------------------------------------------------
    IP7C_Channel::eType Get_Type() { return IP7C_Channel::eTelemetry; }
    void On_Receive(tUINT32 i_dwChannel, 
                    tUINT8 *i_pBuffer, 
                    tUINT32 i_dwSize,
                    tBOOL   i_bBigEndian
                    );
    void On_Status(tUINT32 i_dwChannel, const sP7C_Status *i_pStatus);
    void On_Flush(tUINT32 i_dwChannel, tBOOL *io_pCrash);
};


#endif //UP7_TELEMETRY_TRACE_H