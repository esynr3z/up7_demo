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
#ifndef UP7_CHUNKS_H
#define UP7_CHUNKS_H

#define uP7_CHUNKS_BLOCK_SIZE       128

class CChunks
{
protected:
    struct stBlock
    {
        sP7C_Data_Chunk *pChunks;
        size_t           szChunks;
        size_t           szData;
        stBlock         *pNext;
        stBlock() { pChunks = new sP7C_Data_Chunk[uP7_CHUNKS_BLOCK_SIZE]; szChunks = 0; pNext = 0; szData = 0;}
        ~stBlock() { if (pNext) { delete pNext;} delete pChunks; pChunks = 0; szChunks = 0; szData = 0;}
    };

    struct stIter
    {
        sP7C_Data_Chunk *pHead;
        stBlock         *pCurrentBlock;
    };

    stBlock *m_pChunksHead;
    stBlock *m_pChunksTail;
    stIter   m_stChunkIter; 

public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CChunks()
    {
        m_pChunksHead = new stBlock();
        m_pChunksTail = m_pChunksHead;
        ResetChunkIter();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~CChunks()
    {
        if (m_pChunksHead)
        {
            delete m_pChunksHead;
            m_pChunksHead = NULL;
        }
    }

protected:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    inline void ResetChunkIter()
    {
        SetChunkIter(m_pChunksHead);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    inline void SetChunkIter(stBlock *i_pBlock)
    {
        m_stChunkIter.pHead         = i_pBlock->pChunks;
        m_stChunkIter.pCurrentBlock = i_pBlock;
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    inline void AddChunk(void *i_pData, size_t i_szData)
    {
       m_stChunkIter.pCurrentBlock->szChunks++;
       m_stChunkIter.pCurrentBlock->szData += i_szData;
       m_stChunkIter.pHead->pData  = i_pData;
       m_stChunkIter.pHead->dwSize = (tUINT32)i_szData;
       m_stChunkIter.pHead++;

       if (m_stChunkIter.pCurrentBlock->szChunks >= uP7_CHUNKS_BLOCK_SIZE)
       {
           if (m_stChunkIter.pCurrentBlock->pNext)
           {
               SetChunkIter(m_stChunkIter.pCurrentBlock->pNext);
           }
           else
           {
               m_pChunksTail = new stBlock();
               m_stChunkIter.pCurrentBlock->pNext = m_pChunksTail;
               SetChunkIter(m_pChunksTail);
           }
       }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    inline void ClearChunkHead()
    {
        stBlock *l_pHead = m_pChunksHead;

        m_pChunksHead->szChunks = 0;
        m_pChunksHead->szData   = 0;

        if (m_pChunksHead->pNext)
        {
            m_pChunksTail = m_pChunksHead;
            m_pChunksHead = m_pChunksHead->pNext;
            m_pChunksTail->pNext = NULL;
        }

        if (m_stChunkIter.pCurrentBlock == l_pHead)
        {
            SetChunkIter(m_pChunksHead);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    inline void ClearChunks()
    {
        stBlock *l_pIter = m_pChunksHead;

        while (l_pIter)
        {
            l_pIter->szChunks = 0;
            l_pIter->szData   = 0;
            l_pIter = l_pIter->pNext;
        }

        SetChunkIter(m_pChunksHead);
    }

};

#endif //UP7_CHUNKS_H