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
#ifndef UP7_LINEAR_POOL_H
#define UP7_LINEAR_POOL_H

#define uP7_POOL_DEF_CHUNK_SIZE 16384
#define uP7_POOL_DEF_MAX_SIZE   1048576

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CLinearPool
{
public:
    struct stBuffer
    {
        uint8_t * const pData;
        size_t          szData;

        stBuffer(size_t i_szData) : pData((uint8_t*)malloc(i_szData)), szData(0) {}
        ~stBuffer() { if (pData) { free(pData); } }
    };

protected:
    CBList<stBuffer*> m_cFree;
    CBList<stBuffer*> m_cUsed;
    size_t            m_szChunk;
    size_t            m_szMax;
    size_t            m_szCurrent;

public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CLinearPool(size_t i_szChunkSize, size_t i_szMaxSize)
        : m_szChunk(i_szChunkSize)
        , m_szMax(i_szMaxSize)
        , m_szCurrent(0)
    {
        stBuffer *l_pBuffer = new stBuffer(m_szChunk);
        if (l_pBuffer->pData)
        {
            m_cFree.Push_Last(l_pBuffer);
            m_szCurrent += i_szChunkSize;
        }
        else
        {
            delete l_pBuffer;
            l_pBuffer = NULL;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~CLinearPool()
    {
        m_cFree.Clear(TRUE);
        m_cUsed.Clear(TRUE);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    uint8_t *Use(size_t i_szData)
    {
        if (i_szData > m_szChunk)
        {
            return NULL;
        }

        uint8_t    *l_pReturn = NULL;
        pAList_Cell l_pEl  = m_cUsed.Get_Last();
        stBuffer   *l_pBuf = m_cUsed.Get_Data(l_pEl);

        if (    (l_pBuf)
             && ((m_szChunk - l_pBuf->szData) >= i_szData)
           )
        {
            l_pReturn = l_pBuf->pData + l_pBuf->szData;
            l_pBuf->szData += i_szData;
            return l_pReturn;
        }

        l_pBuf = NULL;
        if (m_cFree.Count())
        {
            l_pBuf = m_cFree.Pull_First();
        }
        else if ((m_szCurrent + m_szChunk) <= m_szMax)
        {
            l_pBuf = new stBuffer(m_szChunk);
            if (l_pBuf->pData)
            {
                m_szCurrent += m_szChunk;
            }
            else
            {
                delete l_pBuf;
                l_pBuf = NULL;
            }
        }

        if (l_pBuf)
        {
            l_pBuf->szData  = i_szData;
            l_pReturn       = l_pBuf->pData;

            m_cUsed.Push_Last(l_pBuf);

            return l_pReturn;
        }

        return NULL;
    }
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CLinearPool::stBuffer * PullUsed()
    {
        if (!m_cUsed.Count())
        {
            return NULL;
        }

        return m_cUsed.Pull_First();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void PushFree(CLinearPool::stBuffer *i_pBuffer)
    {
        if (!i_pBuffer)
        {
            return;
        }

        i_pBuffer->szData = 0;
        m_cFree.Push_Last(i_pBuffer);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void Clear()
    {
        while (m_cUsed.Count())
        {
            stBuffer *l_pBuffer = m_cUsed.Pull_First();
            l_pBuffer->szData = 0;
            m_cFree.Push_Last(l_pBuffer);
        }
    }
};

#endif //UP7_LINEAR_POOL_H