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
#pragma once

#define FJOURNALC_BUFFER_MAX_LENGTH                                    (0x20000)
#define FJOURNALC_DES_MAX_LENGTH                                            (64)

class CJournalC
    : virtual public IJournal
{
private:
    //put volatile variables at the top, to obtain 32 bit alignment. 
    //Project has 8 bytes alignment by default
    volatile LONG      m_lReference_Counter;
    wchar_t           *m_pBuffer;
    SYSTEMTIME         m_tTime;
    wchar_t            m_pTypes_Description[EFJOIRNAL_TYPES_COUNT][FJOURNALC_DES_MAX_LENGTH];
    UINT64             m_pCount[EFJOIRNAL_TYPES_COUNT];
    eFJournal_Type     m_eVerbosity;
    BOOL               m_bInitialized;
    BOOL               m_bError;
    CRITICAL_SECTION   m_tCS;
    BOOL               m_bPrint_Function;

public: 
    CJournalC()
       : m_pBuffer(NULL)
       , m_lReference_Counter(1)
       , m_bInitialized(FALSE)
       , m_bError(FALSE)
       , m_bPrint_Function(FALSE)
       , m_eVerbosity(EFJOIRNAL_TYPE_INFO)
    {
        memset(&m_tCS, 0, sizeof(CRITICAL_SECTION));
        memset(&m_pCount, 0, sizeof(m_pCount));

        InitializeCriticalSection(&m_tCS);

        wcscpy_s(m_pTypes_Description[EFJOIRNAL_TYPE_INFO],     FJOURNALC_DES_MAX_LENGTH, L"INFO    :");
        wcscpy_s(m_pTypes_Description[EFJOIRNAL_TYPE_DEBUG],    FJOURNALC_DES_MAX_LENGTH, L"DEBUG   :");
        wcscpy_s(m_pTypes_Description[EFJOIRNAL_TYPE_WARNING],  FJOURNALC_DES_MAX_LENGTH, L"WARNING :");
        wcscpy_s(m_pTypes_Description[EFJOIRNAL_TYPE_ERROR],    FJOURNALC_DES_MAX_LENGTH, L"ERROR   :");
        wcscpy_s(m_pTypes_Description[EFJOIRNAL_TYPE_CRITICAL], FJOURNALC_DES_MAX_LENGTH, L"CRITICAL:");
    }

    ////////////////////////////////////////////////////////////////////////////
    //Initialize
    tBOOL Initialize(const tXCHAR *i_pName)      
    {
        if (TRUE == m_bInitialized)
        {
            return TRUE;
        }

        if  (TRUE  == m_bError)
        {
            return FALSE;
        }

        CWString l_sDirectory;

        if (FALSE == m_bError)
        {
            m_pBuffer = new wchar_t[FJOURNALC_BUFFER_MAX_LENGTH];
            if (NULL == m_pBuffer)
            {
                m_bError = TRUE;
            }
        }

        if (FALSE == m_bError)
        {
            m_bInitialized = TRUE;
        }

        return (! m_bError);
    }

    void Set_Verbosity(eFJournal_Type i_eVerbosity)
    {
        EnterCriticalSection(&m_tCS);
        m_eVerbosity = i_eVerbosity;
        LeaveCriticalSection(&m_tCS);
    }

    eFJournal_Type Get_Verbosity()
    {
        eFJournal_Type l_eResult = EFJOIRNAL_TYPE_INFO;
        EnterCriticalSection(&m_tCS);
        l_eResult = m_eVerbosity;
        LeaveCriticalSection(&m_tCS);

        return l_eResult;
    }

    tBOOL Log(eFJournal_Type i_eType, 
              const char    *i_pFunction, 
              tUINT32        i_dwLine,
              const tXCHAR  *i_pFormat, 
              ...
             )
    {
        tBOOL    l_bResult  = TRUE;
        int      l_iCount   = 0;
        wchar_t *l_pBuffer  = NULL;
        tUINT32  l_dwUsed   = 0;

        if (    (FALSE == m_bInitialized)
             || (NULL == i_pFormat) 
             || (i_eType >= EFJOIRNAL_TYPES_COUNT) 
           )
        {
            return FALSE;
        }

        EnterCriticalSection(&m_tCS);

        m_pCount[i_eType]++;

        if (i_eType < m_eVerbosity)
        {
            l_bResult = FALSE;
            goto l_lExit;
        }

        if (    (l_bResult) 
             && (i_pFunction) 
             && (m_bPrint_Function)
           )
        {
            l_pBuffer = m_pBuffer + l_dwUsed;

            while (*i_pFunction)
            {
               (*l_pBuffer++) = (*i_pFunction++);
               l_dwUsed ++;
            }

            l_iCount = swprintf_s(m_pBuffer + l_dwUsed, 
                                  FJOURNALC_BUFFER_MAX_LENGTH - l_dwUsed,
                                  L"() Ln %04d : ", i_dwLine
                                 );
            if (0 > l_iCount)
            {
                l_bResult = FALSE;
            }
            else
            {
                l_dwUsed += l_iCount;
            }

            l_pBuffer = m_pBuffer + l_dwUsed;
            (*l_pBuffer++) = 13;
            (*l_pBuffer++) = 10;
            l_dwUsed += 2;
        }

        if (l_bResult)
        {
            GetLocalTime(&m_tTime);
            l_iCount = swprintf_s(m_pBuffer + l_dwUsed, 
                                  FJOURNALC_BUFFER_MAX_LENGTH - l_dwUsed,
                                  L"[%04d.%02d.%02d %02d:%02d:%02d.%03d] %s ",
                                  m_tTime.wYear, m_tTime.wMonth, m_tTime.wDay,
                                  m_tTime.wHour, m_tTime.wMinute, m_tTime.wSecond, m_tTime.wMilliseconds,
                                  m_pTypes_Description[i_eType]
                                 );
            if (0 > l_iCount)
            {
                l_bResult = FALSE;
            }
            else
            {
                l_dwUsed += l_iCount;
            }
        }

        if (l_bResult)
        {
            va_list l_pArgList;
            va_start(l_pArgList, i_pFormat);
            size_t l_stRemaining = 0; 

            if (S_OK == StringCbVPrintfExW(m_pBuffer + l_dwUsed,
                                           (FJOURNALC_BUFFER_MAX_LENGTH - l_dwUsed) * sizeof(wchar_t),
                                           NULL,
                                           &l_stRemaining,
                                           STRSAFE_IGNORE_NULLS,
                                           i_pFormat,
                                           l_pArgList
                                           )
               )
            {
                l_dwUsed = FJOURNALC_BUFFER_MAX_LENGTH - (DWORD)(l_stRemaining / sizeof(wchar_t));
            }
            else
            {
                l_bResult = FALSE;
            }

            va_end(l_pArgList);
        }

        if (l_bResult)
        {
            l_pBuffer = m_pBuffer + l_dwUsed;
            (*l_pBuffer++) = 13;
            (*l_pBuffer++) = 10;
            (*l_pBuffer++) = 0;
            l_dwUsed += 3;

            wprintf(m_pBuffer);
        }

    l_lExit:

        LeaveCriticalSection(&m_tCS);

        return l_bResult;
    }

    tUINT64 Get_Count(eFJournal_Type i_eType)
    {
        UINT64 l_qwReturn = 0ULL;
        if (i_eType >= EFJOIRNAL_TYPES_COUNT) 
        {
            return 0ULL;
        }

        EnterCriticalSection(&m_tCS);
        l_qwReturn = m_pCount[i_eType];
        LeaveCriticalSection(&m_tCS);


        return l_qwReturn;
    }

    tINT32 Add_Ref()
    {
        return InterlockedIncrement(&m_lReference_Counter);
    }

    tINT32 Release()
    {
        volatile tINT32 l_iRCnt = InterlockedDecrement(&m_lReference_Counter);

        if ( 0 >= l_iRCnt)
        {
            delete this;
        }

        return l_iRCnt;
    }

    BOOL Get_Initialized()
    {
        return m_bInitialized;
    }

protected:
    ~CJournalC() //use Release() instead
    {
        if (m_pBuffer)
        {
            delete [] m_pBuffer;
            m_pBuffer = NULL;
        }

        DeleteCriticalSection(&m_tCS);
    }

};
