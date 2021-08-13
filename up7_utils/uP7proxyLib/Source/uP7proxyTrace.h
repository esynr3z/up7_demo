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
#ifndef UP7_PROXY_TRACE_H
#define UP7_PROXY_TRACE_H

#define RESET_UNDEFINED                                           (0xFFFFFFFFUL)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename tData_Type>
class CTemplateDesc
{
    tData_Type *m_pFormat;
    //count of connections drops... see sP7C_Status. If this value and 
    //value from IP7_Client are different - it mean we loose connection
    uint32_t    m_dwResets; 

public:
    CTemplateDesc(tData_Type *i_pFormat)
        : m_pFormat(NULL)
        , m_dwResets(RESET_UNDEFINED)
    {
        if (i_pFormat)
        {
            m_pFormat = (tData_Type *)malloc(i_pFormat->sCommon.dwSize);
            if (m_pFormat)
            {
                memcpy(m_pFormat, i_pFormat, i_pFormat->sCommon.dwSize);
            }
        }
    }

    virtual ~CTemplateDesc()
    {
        if (m_pFormat)
        {
            free(m_pFormat);
            m_pFormat = 0;
        }
    }

    inline void     SetResets(uint32_t i_uResets) { m_dwResets = i_uResets; }
    inline uint32_t GetResets() { return m_dwResets; }
    tData_Type     *GetData()   { return m_pFormat; }
};

typedef CTemplateDesc<sP7Trace_Format> CTraceDesc;
typedef CTemplateDesc<sP7Trace_Module> CModuleDesc;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CProxyTrace
    : public virtual CProxyStream
{                      
    CBList<CTraceDesc*>    m_cTrcDesc;
    CBList<CModuleDesc*>   m_cModDesc;
                          
    sP7Trace_Info          m_sHeader_Info;
    sP7Trace_Utc_Offs      m_sHeader_Utc;

public:
    CProxyTrace(CWString      &i_rName, 
                bool           i_bConvertEndianess,
                CuP7Fifo      *i_pFifo,
                uint8_t        i_bId,
                uint64_t       i_qwFreq,
                IuP7Time      *i_iTime,
                IP7_Trace     *i_pP7Trace,
                IP7_Telemetry *i_pP7Tel,
                IProxyClient  *i_pClient
               );

    virtual ~CProxyTrace();
    bool     ParseDescription(uint32_t i_uSessionId, uint8_t i_uCrc7, const uint8_t *i_pDesc, size_t i_szDesc);
    void     UpdateStartTime(const stProxyPacket *i_pPackets);
    void     Start();
    void     AddModules();
    size_t   Process(const stProxyPacket *i_pPackets);
    uint64_t GetSupportedPacketsMap();


    //----------------------------------------------IP7C_Channel--------------------------------------------------------
    IP7C_Channel::eType Get_Type() { return IP7C_Channel::eTrace; }
    void On_Receive(tUINT32 i_dwChannel, 
                    tUINT8 *i_pBuffer, 
                    tUINT32 i_dwSize,
                    tBOOL   i_bBigEndian
                    );
    void On_Status(tUINT32 i_dwChannel, const sP7C_Status *i_pStatus);
    void On_Flush(tUINT32 i_dwChannel, tBOOL *io_pCrash);
};

#endif //UP7_PROXY_TRACE_H