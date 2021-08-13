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
#ifndef UP7_FIFO_H
#define UP7_FIFO_H

class CuP7FifoGroup;


#define  BUFFER_TAIL_SIZE(iBuffer)         (m_szCpu2HostBuffer - (tUINT32)iBuffer->szUsed)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CuP7Fifo: 
    public IuP7Fifo
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    struct stBuffer
    {
        uint8_t  bCpuId;
        uint8_t *pData;
        size_t   szUsed;

        stBuffer(uint8_t i_bCpuId, size_t i_szBuffer)
        {
            bCpuId = i_bCpuId;
            pData  = (tUINT8*)malloc(i_szBuffer);
            szUsed = 0;
        }

        virtual ~stBuffer()
        {
            if (pData)
            {
                free(pData);
                pData = NULL;
            }
            szUsed  = 0;
        }
    };


protected:
    //put volatile variables at the top, to obtain 32 bit alignment. 
    //Project has 8 bytes alignment by default
    tINT32 volatile        m_lReference;
    uint8_t                m_bCpuId;
    tLOCK                  m_hCS;
    CRingBuffer           *m_pHost2Cpu;
    CMEvent                m_cReadEvent;
    CuP7FifoGroup         *m_pGroup;

    IP7_Trace             *m_pP7Trace;
    IP7_Trace::hModule     m_hP7Mod;

    CBList<stBuffer*>      m_cCpu2Host;
    stBuffer*              m_cCpu2HostCurrent;
    size_t                 m_szCpu2HostBuffer;
    size_t                 m_szCpu2Host;
    size_t                 m_szCpu2HostFree;

public:
    CuP7Fifo(uint8_t i_bCpuId, size_t i_szFifoSize, bool i_bFifoBiDirectional, IP7_Trace *i_pP7Trace);
    virtual ~CuP7Fifo();

    // IuP7Fifo ////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool    Write(const void *i_pData, size_t i_szData); //>m_cCpu2Host, User call
    bool    Write(const IuP7Fifo::stChunk *i_pChunks, size_t i_szChunks, size_t i_szData);
    size_t  GetFreeBytes();

    size_t  Read(void *i_pBuffer, size_t i_szBuffer, uint32_t i_uTimeout); //<m_cHost2Cpu, User call
    int32_t Add_Ref(); 
    int32_t Release();

    // Internals ///////////////////////////////////////////////////////////////////////////////////////////////////////
    bool    Send(const void *i_pData, size_t i_szData); //>m_cHost2Cpu
    void    SetGroup(CuP7FifoGroup *i_pGroup);

    void    PushBuffer(CuP7Fifo::stBuffer *i_pBuffer);
    CuP7Fifo::stBuffer *PullFirst();

    inline uint8_t GetId() const { return m_bCpuId;}
    
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//CFifoList
class CFifoList
    : public CBList<CuP7Fifo*>
{
protected:
    ////////////////////////////////////////////////////////////////////////////
    //CAList::Data_Release
    // override this function in derived class to implement specific data 
    // destructor(structures as example)
    virtual tBOOL Data_Release(CuP7Fifo *i_pData);
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CuP7FifoGroup
{
    friend class CuP7Fifo; 
protected:
    tLOCK        m_hCS;
    CFifoList    m_cFifos;
    CMEvent     *m_pWriteEvent;
    tUINT32      m_uWriteEventId;

    CBList<CuP7Fifo::stBuffer*> m_cCpu2Host;

public:
    CuP7FifoGroup(CMEvent *i_pWriteEvent, tUINT32 i_uWriteEventId);
    virtual ~CuP7FifoGroup();

    bool RegisterFifo(CuP7Fifo *i_pFifo);
    bool UnregisterFifo(CuP7Fifo *i_pFifo);
    CuP7Fifo::stBuffer *PullBuffer();

    inline CFifoList *GetFifos() { return &m_cFifos; }

};



#endif //UP7_FIFO_H
