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
#include "uP7proxy.h"

extern "C" 
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
P7_EXPORT IuP7proxy * __cdecl uP7createProxy(const tXCHAR *i_pArgs, const tXCHAR *i_puP7Dir)
{
    bool       l_bError  = false;
    IuP7proxy *l_pReturn = new CuP7proxy(i_pArgs, i_puP7Dir, l_bError);
    
    if (l_bError)
    {
        l_pReturn->Release();
        l_pReturn = NULL;
    }

    return l_pReturn;
}//uP7createProxy
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CuP7proxy::CuP7proxy(const tXCHAR *i_pArgs, const tXCHAR *i_puP7Dir, bool &o_rError)
    : m_lReference(1)
    , m_pP7Client(NULL)
    , m_pP7Trace(NULL)
    , m_hP7Mod(NULL)
    , m_pP7Tel(NULL)
    , m_pArgs(NULL)
    , m_cClientsReady()
    , m_cClientsFull()
    , m_bProcThread(FALSE)
    , m_hProcThread(0)
{

    m_pP7Client = P7_Create_Client(i_pArgs);
    m_pP7Trace  = P7_Create_Trace(m_pP7Client, TM("MicroP7"));
    m_pP7Tel    = P7_Create_Telemetry(m_pP7Client, TM("MicroP7"));

    if (m_pP7Trace)
    {
        m_pP7Trace->Share(uP7_SHARED_TRACE);
        m_pP7Trace->Register_Module(TM("Proxy"), &m_hP7Mod);
    }

    if (m_pP7Tel)
    {
        m_pP7Tel->Share(uP7_SHARED_TELEMETRY);
    }

    o_rError = false;
    if (i_pArgs)
    {
        m_pArgs = PStrDub(i_pArgs);
        if (!m_pArgs)
        {
            uERROR(TM("Memory allocation error {%s}"), i_pArgs);
            o_rError = true;
        }
    }

    if (!o_rError)
    {
        static_assert(eProcThreadEventsCount == 5, "Events count is unexpected");
        if (FALSE == m_cProcEvent.Init(eProcThreadEventsCount, 
                                       EMEVENT_MULTI, //eProcThreadExit
                                       EMEVENT_MULTI, //eProcThreadAddCpu,
                                       EMEVENT_MULTI, //eProcThreadDelCpu,
                                       EMEVENT_MULTI, //eProcThreadCpuData,
                                       EMEVENT_MULTI  //eProcThreadMaintain,
                                      )
           )
        {
            uERROR(TM("Can't init event"), 0);
            o_rError = true;
        }
    }

    if (!o_rError)
    {
        static_assert(eManageThreadEventsCount == 1, "Events count is unexpected");
        if (FALSE == m_cManageEvent.Init(eManageThreadEventsCount, 
                                       EMEVENT_SINGLE_AUTO //eManageThreadExit
                                      )
           )
        {
            uERROR(TM("Can't init event"), 0);
            o_rError = true;
        }
    }

    //scanning pre-processor folder for files and store them as Red-Black tree using session ID as a key.
    if (!o_rError)
    {
        size_t            l_szCount = 0;
        CBList<CWString*> l_cuP7files;
        CWString          l_cDir(i_puP7Dir);
        CFSYS::Enumerate_Files(&l_cuP7files, &l_cDir, TM("*.") uP7_FILES_EXT);

        while (l_cuP7files.Count())
        {
            CWString *l_pFileName = l_cuP7files.Pull_First();
            CPFile    l_cFile;

            if (l_cFile.Open(l_pFileName->Get(), IFile::EOPEN | IFile::EACCESS_READ | IFile::ESHARE_READ))
            {
                stuP7SessionFileHeader l_stHdr = {0};

                if (sizeof(l_stHdr) == l_cFile.Read((tUINT8*)&l_stHdr, sizeof(l_stHdr)))
                {
                    if (l_stHdr.uVersion == SESSION_ID_FILE_VER)
                    {
                        stPreProcessorFile *l_pFile = m_cFiles.Find(l_stHdr.uSessionId);
                        if (!l_pFile)
                        {
                            m_cFiles.Push(new stPreProcessorFile(l_pFileName, l_stHdr.uSessionId, l_stHdr.uCrc7), 
                                          l_stHdr.uSessionId);
                            l_pFileName = NULL;
                            l_szCount ++;
                        }
                        else
                        {
                            uERROR(TM("Session ID collision for files: {%s}<>{%s}"), 
                                   l_pFileName->Get(), 
                                   l_pFile->pFileName->Get());
                        }
                    }
                    else
                    {
                        uERROR(TM("Session file version is unsupported: {%s} ver:%u"), 
                               l_pFileName->Get(), 
                               l_stHdr.uVersion);
                    }
                }
                else
                {
                    uERROR(TM("Can't read session ID from preprocessor file {%s}"), l_pFileName->Get());
                }

                l_cFile.Close(FALSE);
            }
            else
            {
                uERROR(TM("Can't open pre-processor file: {%s}"), l_pFileName->Get());
            }

            if (l_pFileName)
            {
                delete l_pFileName;
                l_pFileName = NULL;
            }
        }

        if (!l_szCount)
        {
            uERROR(TM("Preprocessor folder seems to be empty: {%s}"), l_cDir.Get()); 
            o_rError = true;
        }

        l_cuP7files.Clear(FALSE);
    }

    if (!o_rError)
    {
        if (CThShell::Create(&StaticProcRoutine, this, &m_hProcThread, TM("uP7")))
        {
            m_bProcThread = TRUE;
        }

        if (CThShell::Create(&StaticManageRoutine, this, &m_hManageThread, TM("uP7")))
        {
            m_bManageThread = TRUE;
        }
    }
}




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CuP7proxy::~CuP7proxy()
{
    if (m_bProcThread)
    {
        m_cProcEvent.Set(eProcThreadExit);
        if (TRUE == CThShell::Close(m_hProcThread, 15000))
        {
            m_bProcThread = FALSE;
            m_hProcThread = 0;
        }
        else
        {
            uERROR(TM("MicroP7 thread timeout!"), 0);
        }
    }

    if (m_bManageThread)
    {
        m_cManageEvent.Set(eManageThreadExit);
        if (TRUE == CThShell::Close(m_hManageThread, 1500))
        {
            m_bManageThread = FALSE;
            m_hManageThread = 0;
        }
        else
        {
            uERROR(TM("MicroP7 thread timeout!"), 0);
        }
    }

    if (m_pArgs)
    {
        PStrFreeDub(m_pArgs);
        m_pArgs = NULL;
    }

    m_cClientsReady.Clear(TRUE);
    m_cClientsFull.Clear(TRUE);

    m_cLock.Lock();
    m_cCmdList.Clear(TRUE);
    m_cLock.Unlock();

    SAFE_RELEASE(m_pP7Tel);
    SAFE_RELEASE(m_pP7Trace);
    SAFE_RELEASE(m_pP7Client);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CuP7proxy::RegisterCpu(uint8_t       i_bCpuId, 
                            bool          i_bBigEndian, 
                            uint64_t      i_qwFreq, 
                            const tXCHAR *i_pName,
                            size_t        i_szFifoLen,
                            bool          i_bFifoBiDirectional,
                            IuP7Fifo    *&o_iFifo)
{
    if (i_szFifoLen < FIFO_MIN_LENGTH)
    {
        uERROR(TM("FIFO size is too small %zu (<%zu)"), i_szFifoLen, (size_t)FIFO_MIN_LENGTH); 
        return false;
    }

    m_cLock.Lock();

    bool      l_bReturn = false;
    CuP7Fifo *l_pFifo   = new CuP7Fifo(i_bCpuId, i_szFifoLen, i_bFifoBiDirectional, m_pP7Trace);
    stCmd     l_cCmd(eProcThreadAddCpu, i_bCpuId);

    l_cCmd.cmdAdd.pFifo      = l_pFifo;
    l_cCmd.cmdAdd.bBigEndian = i_bBigEndian;
    l_cCmd.cmdAdd.qwFreq     = i_qwFreq;
    l_cCmd.cmdAdd.iTime      = NULL;
    l_cCmd.bResult           = false;
    l_cCmd.cmdAdd.pName->Set(i_pName);

    m_cCmdList.Add_After(m_cCmdList.Get_Last(), &l_cCmd);
    m_cLock.Unlock();

    m_cProcEvent.Set(eProcThreadAddCpu);

    if (0 == l_cCmd.cEvent.Wait(COMMAND_TIMEOUT_MS))
    {
        m_cLock.Lock();
        if (l_cCmd.bResult)    
        {
            o_iFifo = l_pFifo;
            l_pFifo->Add_Ref();
            l_bReturn = true;
        }
        m_cLock.Unlock();
    }

    l_pFifo->Release();
    l_pFifo = NULL;

    return l_bReturn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CuP7proxy::RegisterCpu(uint8_t       i_bCpuId, 
                            bool          i_bBigEndian, 
                            IuP7Time     *i_iTime, 
                            const tXCHAR *i_pName,
                            size_t        i_szFifoLen,
                            bool          i_bFifoBiDirectional,
                            IuP7Fifo    *&o_iFifo)
{
    if (i_szFifoLen < FIFO_MIN_LENGTH)
    {
        uERROR(TM("FIFO size is too small %zu (<%zu)"), i_szFifoLen, (size_t)FIFO_MIN_LENGTH); 
        return false;
    }

    if (!i_iTime)
    {
        uERROR(TM("Time object is NULL"), 0); 
        return false;
    }

    m_cLock.Lock();

    bool      l_bReturn = false;
    CuP7Fifo *l_pFifo   = new CuP7Fifo(i_bCpuId, i_szFifoLen, i_bFifoBiDirectional, m_pP7Trace);
    stCmd     l_cCmd(eProcThreadAddCpu, i_bCpuId);

    l_cCmd.cmdAdd.pFifo      = l_pFifo;
    l_cCmd.cmdAdd.bBigEndian = i_bBigEndian;
    l_cCmd.cmdAdd.qwFreq     = i_iTime->GetFrequency();
    l_cCmd.cmdAdd.iTime      = i_iTime;
    l_cCmd.bResult           = false;
    l_cCmd.cmdAdd.pName->Set(i_pName);

    m_cCmdList.Add_After(m_cCmdList.Get_Last(), &l_cCmd);
    m_cLock.Unlock();

    m_cProcEvent.Set(eProcThreadAddCpu);

    if (0 == l_cCmd.cEvent.Wait(COMMAND_TIMEOUT_MS))
    {
        m_cLock.Lock();
        if (l_cCmd.bResult)    
        {
            o_iFifo = l_pFifo;
            l_pFifo->Add_Ref();
            l_bReturn = true;
        }
        m_cLock.Unlock();
    }

    l_pFifo->Release();
    l_pFifo = NULL;

    return l_bReturn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CuP7proxy::UnRegisterCpu(uint8_t i_bCpuId)
{
    m_cLock.Lock();
    bool  l_bReturn = false;
    stCmd l_cCmd(eProcThreadDelCpu, i_bCpuId);
    m_cCmdList.Add_After(m_cCmdList.Get_Last(), &l_cCmd);
    m_cLock.Unlock();

    m_cProcEvent.Set(eProcThreadDelCpu);

    if (0 == l_cCmd.cEvent.Wait(COMMAND_TIMEOUT_MS))
    {
        m_cLock.Lock();
        l_bReturn = l_cCmd.bResult;
        m_cLock.Unlock();
    }


    return l_bReturn;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IP7_Client *CuP7proxy::RegisterChannel(IP7C_Channel *i_pChannel)
{
    CLock l_cLock(&m_cLock);

    if (!i_pChannel)
    {
        return NULL;
    }

    eClient_Status l_eStatus = ECLIENT_STATUS_OK;
    pAList_Cell    l_pEl     = m_cClientsReady.Get_First();
    IP7_Client    *l_pClient = NULL;

    while (l_pEl)
    {
        l_pClient = m_cClientsReady.Get_Data(l_pEl);
        if (l_pClient)
        {
            l_eStatus = l_pClient->Register_Channel(i_pChannel);
            if (ECLIENT_STATUS_OK == l_eStatus)
            {
                break;
            }
            else
            {
                pAList_Cell l_pNext = m_cClientsReady.Get_Next(l_pEl);    
                m_cClientsReady.Del(l_pEl, FALSE);
                m_cClientsFull.Add_After(NULL, l_pClient);
                l_pEl     = l_pNext;
                l_pClient = NULL;
            }
        }
    }
    
    if (!l_pClient)
    {
        l_pClient = P7_Create_Client(m_pArgs); 
        if (l_pClient)
        {
            l_eStatus = l_pClient->Register_Channel(i_pChannel);
            if (ECLIENT_STATUS_OK == l_eStatus)
            {
                m_cClientsReady.Add_After(NULL, l_pClient);
            }
            else
            {
                l_pClient->Release();
                l_pClient = NULL;
            }
        }
    }

    return l_pClient;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CuP7proxy::ReleaseChannel(IP7_Client *i_pClient, uint32_t i_uID)
{
    CLock l_cLock(&m_cLock);
    if (!i_pClient)
    {
        return;
    }

    if (ECLIENT_STATUS_OK == i_pClient->Unregister_Channel(i_uID))
    {
        pAList_Cell l_pEl = NULL;
        while ((l_pEl = m_cClientsFull.Get_Next(l_pEl)))
        {
            if (i_pClient == m_cClientsFull.Get_Data(l_pEl))    
            {
                m_cClientsFull.Del(l_pEl, FALSE);
                m_cClientsReady.Add_After(NULL, i_pClient);
                i_pClient = NULL;
                break;
            }
        }
    }
    else
    {
        uERROR(TM("uP7 proxy can't unregister channel %u"), i_uID);
    }

    if (!i_pClient)
    {
        uERROR(TM("uP7 proxy can't find client"), 0);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t CuP7proxy::Add_Ref()
{
    return ATOMIC_INC(&m_lReference);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t CuP7proxy::Release()
{
    tINT32 l_lResult = ATOMIC_DEC(&m_lReference);
    if ( 0 >= l_lResult )
    {
        delete this;
    }

    return l_lResult;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CuP7proxy::IsBigEndian()
{
    tUINT16 l_sValue = 0x0001;
    tUINT8 *l_pValue = (tUINT8*)&l_sValue;
    return (0 == l_pValue[0]);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CuP7proxy::ProcRoutine()
{
    tBOOL               l_bExit       = FALSE;
    tUINT32             l_uCmdId      = 0;
    uint8_t             l_bCpuId      = 0;
    CProxyCpu          *l_pCpus[uP7_CPU_MAX_COUNT];
    CBList<CProxyCpu *> l_cCpuList;
    CuP7FifoGroup       l_cFifoGroup(&m_cProcEvent, eProcThreadCpuData);
    CuP7Fifo::stBuffer *l_pBuffer     = NULL;
    const uint32_t      l_uTimeout    = 100;

    if (m_pP7Trace)
    {
        m_pP7Trace->Register_Thread(TM("Proxy"), 0); 
    }

    memset(l_pCpus, 0, sizeof(l_pCpus));

    while (FALSE == l_bExit)
    {
        l_uCmdId = m_cProcEvent.Wait(l_uTimeout);

        if (eProcThreadCpuData == l_uCmdId)
        {
            l_pBuffer = l_cFifoGroup.PullBuffer();
            if (l_pBuffer)    
            {
                if (l_pCpus[l_pBuffer->bCpuId])
                {
                    l_pCpus[l_pBuffer->bCpuId]->Process(l_pBuffer);
                }
                else
                {
                    uERROR(TM("Reveive empty buffer, panic?"), 0);
                }
            }
            else
            {
                uCRITICAL(TM("Unitialized cpu#%d, function IuP7proxy::RegisterCpu() wasn't called?"), (int)l_bCpuId); 
            }
        }
        else if (    (eProcThreadAddCpu == l_uCmdId)
                  || (eProcThreadDelCpu == l_uCmdId)
                )
        {
            CLock l_cLock(&m_cLock);
            if (m_cCmdList.Count())
            {
                stCmd *l_pCmd = m_cCmdList.Pull_First();
                if (eProcThreadAddCpu == l_pCmd->eIdl)
                {
                    if (!l_pCpus[l_pCmd->bCpuId])
                    {
                        l_cFifoGroup.RegisterFifo(l_pCmd->cmdAdd.pFifo);

                        l_pCpus[l_pCmd->bCpuId] = new CProxyCpu(*l_pCmd->cmdAdd.pName, 
                                                                IsBigEndian() != l_pCmd->cmdAdd.bBigEndian, 
                                                               &m_cFiles, 
                                                                l_pCmd->cmdAdd.pFifo,
                                                                l_pCmd->bCpuId,
                                                                l_pCmd->cmdAdd.qwFreq,
                                                                l_pCmd->cmdAdd.iTime,
                                                                m_pP7Trace, 
                                                                m_pP7Tel,
                                                                this
                                                               );
                        l_cCpuList.Push_Last(l_pCpus[l_pCmd->bCpuId]);
                        l_pCmd->bResult = true;
                    }
                    else
                    {
                        uERROR(TM("uP7 proxy for cpu#%d already created!"), (int)l_pCmd->bCpuId); 
                    }

                }
                else if (eProcThreadDelCpu == l_pCmd->eIdl)
                {
                    pAList_Cell l_pEl = NULL;
                    while ((l_pEl = l_cCpuList.Get_Next(l_pEl)))
                    {
                        CProxyCpu *l_pCpu = l_cCpuList.Get_Data(l_pEl);
                        if (    (l_pCpu)
                             && (l_pCmd->bCpuId == l_pCpu->GetId())
                           )
                        {
                            l_cCpuList.Del(l_pEl, TRUE);
                            l_pCmd->bResult = true;
                            break;
                        }
                    }

                    l_pCpus[l_pCmd->bCpuId] = NULL;
                }

                l_pCmd->cEvent.Set(0);
            }
        }
        else if (eProcThreadMaintain == l_uCmdId)
        {
            pAList_Cell l_pEl = NULL;
            while ((l_pEl = l_cCpuList.Get_Next(l_pEl)))
            {
                CProxyCpu *l_pCpu = l_cCpuList.Get_Data(l_pEl);
                if (l_pCpu)
                {
                    l_pCpu->Maintain();
                }
            }
        }
        else if (eProcThreadExit == l_uCmdId)
        {
            l_bExit = TRUE;
        }
        else // (MEVENT_TIME_OUT == l_uCmdId)
        {
            CFifoList  *l_pFifos = l_cFifoGroup.GetFifos();
            pAList_Cell l_pEl   = NULL;

            while ((l_pEl = l_pFifos->Get_Next(l_pEl)))
            {
                CuP7Fifo::stBuffer* l_pBuffer = l_pFifos->Get_Data(l_pEl)->PullFirst();
                if (l_pBuffer)    
                {
                    if (l_pCpus[l_pBuffer->bCpuId])
                    {
                        l_pCpus[l_pBuffer->bCpuId]->Process(l_pBuffer);
                    }
                    else
                    {
                        uCRITICAL(TM("Unitialized cpu#%d, function IuP7proxy::RegisterCpu() wasn't called?"), (int)l_bCpuId); 
                    }
                }
            }
        }
    }

    l_cCpuList.Clear(TRUE);

    for (size_t l_szI = 0; l_szI < LENGTH(l_pCpus); l_szI++ )
    {
        l_pCpus[l_szI] = NULL;
    }

    if (m_pP7Trace) 
    {
        m_pP7Trace->Unregister_Thread(0);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CuP7proxy::ManageRoutine()
{
    tBOOL    l_bExit       = FALSE;
    uint32_t l_uMaintainTs = GetTickCount();
    tUINT32  l_uCmdId      = 0;
    tUINT32  l_uTimeout    = 100;


    if (m_pP7Trace)
    {
        m_pP7Trace->Register_Thread(TM("Proxy"), 0); 
    }

    while (FALSE == l_bExit)
    {
        l_uCmdId = m_cManageEvent.Wait(l_uTimeout);

        if (eManageThreadExit == l_uCmdId)
        {
            l_bExit = TRUE;
        }

        if (CTicks::Difference(GetTickCount(), l_uMaintainTs) >= uP7_MAINTAIN_TIME_MS)
        {
            m_cProcEvent.Set(eProcThreadMaintain);
            l_uMaintainTs = GetTickCount();
        }
    }

    if (m_pP7Trace) 
    {
        m_pP7Trace->Unregister_Thread(0);
    }
}