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
#ifndef UP7_PROXY_H
#define UP7_PROXY_H

#define uP7_MAX_BUFFER_SZIE    0x40000

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CClientsList
    : public CListPool<IP7_Client*>
{
public:
    CClientsList() : CListPool<IP7_Client*>(8){}
protected:
    virtual tBOOL Data_Release(IP7_Client *i_pData)
    {
        if (NULL == i_pData) { return FALSE; }
        i_pData->Release();
        return TRUE;
    }
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CuP7proxy
    : public IuP7proxy
    , public IProxyClient
{
    enum eProcThread
    {
        eProcThreadUnk  = -1, 
        eProcThreadExit =  0, 
        eProcThreadAddCpu,
        eProcThreadDelCpu,
        eProcThreadCpuData,
        eProcThreadMaintain,
        eProcThreadEventsCount
    };

    enum eManageThread
    {
        eManageThreadUnk  = -1, 
        eManageThreadExit =  0, 
        eManageThreadEventsCount
    };

    struct stCmd
    {
        eProcThread eIdl;
        uint8_t     bCpuId;
        bool        bResult;
        CMEvent     cEvent;
        union 
        {
            struct
            {
                CuP7Fifo   *pFifo;
                bool        bBigEndian;
                uint64_t    qwFreq;
                IuP7Time   *iTime;
                CWString   *pName;
            } cmdAdd;
            //struct
            //{
            //} cmdDel;
        };

        stCmd(eProcThread i_eIdl, uint8_t i_bCpuId)
        {
            eIdl    = i_eIdl;
            bCpuId  = i_bCpuId;
            bResult = false;
            cEvent.Init(1, EMEVENT_SINGLE_AUTO);

            if (eProcThreadAddCpu == eIdl)
            {
                cmdAdd.bBigEndian = false;
                cmdAdd.pFifo      = NULL;
                cmdAdd.pName      = new CWString();
                cmdAdd.qwFreq     = 0ull;
            }
        }

        virtual ~stCmd()
        {
            if (eProcThreadAddCpu == eIdl)
            {
                if (cmdAdd.pName)
                {
                    delete cmdAdd.pName;
                }
            }
        }

    };

    class CCmdList : public CListPool<CuP7proxy::stCmd*>
    {
    public:    CCmdList() : CListPool<CuP7proxy::stCmd*>(8){}
    protected: virtual tBOOL Data_Release(CuP7proxy::stCmd* i_stData) { if (i_stData) delete i_stData; return TRUE; }
    };


protected:
    //put volatile variables at the top, to obtain 32 bit alignment. 
    //Project has 8 bytes alignment by default
    tINT32 volatile        m_lReference;
                          
    IP7_Client            *m_pP7Client;
    IP7_Trace             *m_pP7Trace;
    IP7_Trace::hModule     m_hP7Mod;
    IP7_Telemetry         *m_pP7Tel;
                          
    CLock                  m_cLock;
                          
    tXCHAR                *m_pArgs;
                          
    CClientsList           m_cClientsReady;
    CClientsList           m_cClientsFull;
    CPreProcessorFilesTree m_cFiles;

    CMEvent                m_cProcEvent;
    tBOOL                  m_bProcThread;
    CThShell::tTHREAD      m_hProcThread;
    CCmdList               m_cCmdList;

    CMEvent                m_cManageEvent;
    tBOOL                  m_bManageThread;
    CThShell::tTHREAD      m_hManageThread;

public:
    CuP7proxy(const tXCHAR *i_pArgs, const tXCHAR *i_puP7Dir, bool &o_rError);
    virtual ~CuP7proxy();

    virtual bool    RegisterCpu(uint8_t       i_bCpuId, 
                                bool          i_bBigEndian, 
                                uint64_t      i_qwFreq, 
                                const tXCHAR *i_pName,
                                size_t        i_szFifoLen,
                                bool          i_bFifoBiDirectional,
                                IuP7Fifo    *&o_iFifo
                               );

    virtual bool RegisterCpu(uint8_t       i_bCpuId, 
                             bool          i_bBigEndian, 
                             IuP7Time     *i_iTime, 
                             const tXCHAR *i_pName,
                             size_t        i_szFifoLen,
                             bool          i_bFifoBiDirectional,
                             IuP7Fifo    *&o_iFifo
                            );

    virtual bool    UnRegisterCpu(uint8_t i_bCpuId);
    virtual int32_t Add_Ref();
    virtual int32_t Release();

    //IProxyClient
    IP7_Client     *RegisterChannel(IP7C_Channel *i_pChannel);
    void            ReleaseChannel(IP7_Client *i_pClient, uint32_t i_uID);

private:
    bool IsBigEndian();

    static THSHELL_RET_TYPE THSHELL_CALL_TYPE StaticProcRoutine(void *i_pContext)
    {
        CuP7proxy *l_pRoutine = static_cast<CuP7proxy *>(i_pContext);
        if (l_pRoutine)
        {
            l_pRoutine->ProcRoutine();
        }

        CThShell::Cleanup();
        return THSHELL_RET_OK;
    } 

    void ProcRoutine();

    static THSHELL_RET_TYPE THSHELL_CALL_TYPE StaticManageRoutine(void *i_pContext)
    {
        CuP7proxy *l_pRoutine = static_cast<CuP7proxy *>(i_pContext);
        if (l_pRoutine)
        {
            l_pRoutine->ManageRoutine();
        }

        CThShell::Cleanup();
        return THSHELL_RET_OK;
    } 

    void ManageRoutine();

};

#endif //UP7_PROXY_H