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
#ifndef UP7_PRE_TOOLS_H
#define UP7_PRE_TOOLS_H

static const char *g_pErrorsText[eErrorMaxValue] = 
{
    "No error!",                                //eErrorNo            
    "Console argument is wrong",                //eErrorArguments     
    "XML parsing error",                        //eErrorXmlParsing    
    "File open error",                          //eErrorFileOpen      
    "File is too big, max size is 32MB",        //eErrorFileTooBig    
    "Can't read file",                          //eErrorFileRead      
    "Function argument(s) is wrong",            //eErrorFunctionArgs  
    "Not a function",                           //eErrorNotFunction   
    "Trace ID is wrong, should contains "
      "only characters from 0..9",              //eErrorTraceId       
    "Trace format string is wrong, should"
      " contain only constant string, no "
      "macro of functions calls are allowed!",  //eErrorTraceFormat   
    "Memory allocation problem",                //eErrorMemAlloc      
    "Max trace ID (65535) is reached",          //eErrorTraceIdOverFlow
    "Trace ID is duplicated for two read-only"
      " files, unfortunately uP7 preprocessor "
      "can't fix it in auto-mode",              //eErrorTraceIdDuplicate
    "can't write file!",                        //eErrorFileWrite
    "Telemetry counter is duplicated"           //eErrorNameDuplicated
};

static_assert(LENGTH(g_pErrorsText) == eErrorMaxValue, "Some error codes aren't described!");


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static UNUSED_FUNC uint64_t ntohqw(uint64_t i_qwX)
{
#if defined(_WIN32) || defined(_WIN64)
    return _byteswap_uint64(i_qwX);
#else
    i_qwX = (i_qwX & 0x00000000FFFFFFFFull) << 32 | (i_qwX & 0xFFFFFFFF00000000ull) >> 32;
    i_qwX = (i_qwX & 0x0000FFFF0000FFFFull) << 16 | (i_qwX & 0xFFFF0000FFFF0000ull) >> 16;
    return (i_qwX & 0x00FF00FF00FF00FFull) << 8  | (i_qwX & 0xFF00FF00FF00FF00ull) >> 8;
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static UNUSED_FUNC void ntohstr(tWCHAR *l_pBegin, tWCHAR *l_pEnd)
{
    while (    (*l_pBegin)
            && (l_pBegin < l_pEnd)
            )
    {
        *l_pBegin = ntohs(*l_pBegin);

        l_pBegin ++;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static UNUSED_FUNC void ntohstr(tWCHAR *l_pBegin)
{
    while (*l_pBegin)
    {
        *l_pBegin = ntohs(*l_pBegin);
        l_pBegin ++;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static UNUSED_FUNC tXCHAR *NormalizePath(const tXCHAR *io_pPath)
{

    if (!io_pPath)
    {
        return NULL;
    }

    tXCHAR       *l_pDst       = PStrDub(io_pPath);
    tXCHAR       *l_pRet       = l_pDst;
    const tXCHAR *l_pSrc       = io_pPath;
    tBOOL         l_bPrevSlash = FALSE;

    if (!l_pDst)
    {
        return l_pRet;
    }

    while (*l_pSrc)
    {

        if (    (TM('/') == *l_pSrc)
             || (TM('\\') == *l_pSrc)
           )
        {
            if (!l_bPrevSlash)
            {
                *l_pDst = *l_pSrc;

                if (TM('\\') == *l_pDst)
                {
                    *l_pDst = TM('/');
                }

                l_pDst ++;
            }

            l_bPrevSlash = TRUE;
        }
        else
        {
            *l_pDst = *l_pSrc;
            l_pDst ++;

            l_bPrevSlash = FALSE;
        }


        l_pSrc++;
    }

    *l_pDst = 0;

    return l_pRet;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename ValType>
static ValType *GetStringEnd(ValType *i_pStart)
{
    if ('\"' != *i_pStart)
    {
        return NULL;
    }

    i_pStart++;

    bool l_bBackSlash = false;  

    while (*i_pStart)
    {
        if (!l_bBackSlash)
        {
            if ('\"' == *i_pStart)
            {
                break;
            }
            else if ('\\' == *i_pStart)
            {
                l_bBackSlash = true;
            }
        }
        else 
        {
            l_bBackSlash = false;
        }

        i_pStart++;
    }

    if (!*i_pStart)
    {
        i_pStart = NULL;
    }

    return i_pStart;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CUtf8
{
    char  *m_pUtf8;
    size_t m_szUtf8Max;
    size_t m_szUtf8;
public:
    CUtf8(const tXCHAR *i_pBuffer)
        : m_pUtf8(NULL)
        , m_szUtf8Max(0)
        , m_szUtf8(0)
    {
        if (!i_pBuffer)
        {
            return;
        }

    #ifdef UTF8_ENCODING
        m_pUtf8  = const_cast<char*>(i_pBuffer);    
        m_szUtf8 = strlen(m_pUtf8);
    #else
        m_szUtf8Max = PStrLen(i_pBuffer) * 5;
        m_pUtf8 = (char*)malloc(m_szUtf8Max);
        if (m_pUtf8)
        {
            int l_iCount = Convert_UTF16_To_UTF8((const tWCHAR *)i_pBuffer, m_pUtf8, (tUINT32)m_szUtf8Max);
            if (l_iCount < 0) 
            {
                m_pUtf8[0] = 0;
            }
            else
            {
                m_szUtf8 = (size_t)l_iCount;
            }
        }
    #endif         
    }

    const char *text()
    {
        return m_pUtf8;
    }

    size_t length()
    {
        return m_szUtf8;
    }

    ~CUtf8()
    {
        if (    (m_szUtf8Max)
             && (m_pUtf8)
           )
        {
            free(m_pUtf8);
            m_pUtf8 = NULL;
        }
    }
};

#endif //UP7_PRE_TOOLS_H