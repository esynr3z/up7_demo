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
#include "uP7preCommon.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CFuncRoot::CFuncRoot(CpreFile *i_pFile, stFuncDesc *i_pDesc, const char *i_pStart, const char *i_pStop, int i_iLine, const char *i_pFunctionName)
    : m_pFile(i_pFile)
    , m_pDesc(i_pDesc)
    , m_iLine(i_iLine)
    , m_pFunction(OSSTRDUP(i_pFunctionName))
    , m_eError(eErrorNo)
    , m_bUpdated(false)
{
    i_pStart += m_pDesc->szName;

    //look for (
    while (i_pStart < i_pStop)
    {
        if ('(' == *i_pStart)
        {
            i_pStart++;
            break;
        }
        else if (    (' '  != *i_pStart)
                  && ('\t' != *i_pStart)
                  && ('\n' != *i_pStart)
                  && ('\r' != *i_pStart)
                )
        {
            m_eError = eErrorNotFunction;
            break;
        }

        i_pStart++;         
    }

    //look for )
    while (i_pStart < i_pStop)
    {
        if (')' == *i_pStop)
        {
            i_pStop++;
            break;
        }
        else if (    (' '  != *i_pStop)
                  && ('\t' != *i_pStop)
                  && ('\n' != *i_pStop)
                  && ('\r' != *i_pStop)
                  && (';'  != *i_pStop)
                )
        {
            m_eError = eErrorNotFunction;
            break;
        }

        i_pStop--;         
    }

    if (eErrorNo != m_eError)
    {
        return;
    }

    while (i_pStart < i_pStop)
    {
        const char *l_pRefStart  = i_pStart;
        int         l_iBrackets  = 0;


        while (i_pStart < i_pStop)
        {
            if ('(' == *i_pStart) ++l_iBrackets;
            else if (')' == *i_pStart) --l_iBrackets;
            else if ('\"' == *i_pStart)
            {
                i_pStart = GetStringEnd(i_pStart);
                if (!i_pStart)
                {
                    m_eError = eErrorFunctionArgs;
                    break;
                }
            }
            else if (    (0 == l_iBrackets)
                      && (',' == *i_pStart)
                    )
            {
                break;
            }

            i_pStart++;
        }

        if (    (eErrorNo == m_eError)
             && (i_pStart)
           )
        {
            if (    (',' == *i_pStart)
                 || (i_pStart == i_pStop)
               )
            {
                m_cArgs.Push_Last(new stArg(l_pRefStart, i_pStart - 1));
            }
            else
            {
                m_eError = eErrorFunctionArgs;
                break;
            }
        }
        else
        {
            break;
        }

        i_pStart++;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CFuncRoot::~CFuncRoot()
{
    if (m_pFunction)
    {
        free(m_pFunction);
        m_pFunction = NULL;
    }

    m_cArgs.Clear(TRUE);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
eErrorCodes CFuncRoot::GetError()   
{
    return m_eError; 
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CFuncRoot::GetLine()
{
    return m_iLine; 
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* CFuncRoot::GetFilePath()  
{
    return m_pFile->GetPath(); 
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* CFuncRoot::GetFunction()  
{
    return m_pFunction; 
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CFuncRoot::IsUpdated()
{
    return m_bUpdated;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
char* CFuncRoot::FindFunctionEnd(char *i_pStart, size_t i_szNameLen, int &o_rLines)
{
    //skipping name
    while (    (i_szNameLen)
            && (*i_pStart)
          )
    {
        i_szNameLen --;
        i_pStart++;
    }

    if (!*i_pStart)
    {
        return NULL;
    }


    //looking for (
    while (*i_pStart)
    {
        if ('(' == *i_pStart)
        {
            i_pStart++;
            break;
        }
        else if (    (' '  == *i_pStart)
                  || ('\t' == *i_pStart)
                  || ('\r' == *i_pStart)
                )
        {
            i_pStart++;    
        }
        else if (0xA == *i_pStart)
        {
            i_pStart++;    
            o_rLines++;
        }
        else
        {
            return NULL;
        }
    }

    int l_iBrackets  = 1;

    //looking for last )
    while (*i_pStart)
    {
        if ('(' == *i_pStart)
        {
            l_iBrackets++;
        }
        else if (')' == *i_pStart)
        {
            l_iBrackets--;
            if (!l_iBrackets)
            {
                //found end of the function
                break;
            }
        }
        else if (0xA == *i_pStart)
        {
            o_rLines++;
        }
        else if ('\"' == *i_pStart)
        {
            i_pStart = GetStringEnd(i_pStart);
            if (!i_pStart)
            {
                break;
            }
        }

        i_pStart++;
    }

    if (    (i_pStart)
         && (!*i_pStart)
       )
    {
        i_pStart = NULL;
    }

    return i_pStart;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
char *CFuncRoot::GetParameterStrValue(const stArg *i_pArg)
{
    if (eErrorNo != m_eError)
    {
        return NULL;
    }

    if (!i_pArg)
    {
        m_eError = eErrorFunctionArgs;
        return NULL;
    }

    size_t l_szFormat = i_pArg->pRefStop - i_pArg->pRefStart + 1;
    char  *l_pReturn  = (char*) malloc(l_szFormat + 1);

    if (l_pReturn)
    {
        const char *l_pItSrc = i_pArg->pRefStart;
        char       *l_pItDst = l_pReturn;

        while (l_pItSrc <= i_pArg->pRefStop)
        {

            //searching starting "
            while (l_pItSrc <= i_pArg->pRefStop)
            {
                if ('"' == *l_pItSrc)
                {
                    l_pItSrc++;    
                    break;
                }
                else if (    (' '  == *l_pItSrc)
                          || ('\t' == *l_pItSrc)
                          || ('\r' == *l_pItSrc)
                          || ('\n' == *l_pItSrc)
                        )
                {
                    l_pItSrc++;    
                }
                else
                {
                    if (    (')' == *l_pItSrc)
                         && (l_pItSrc == i_pArg->pRefStop)
                       )
                    {
                        l_pItSrc ++;
                    }
                    else
                    {
                        m_eError = eErrorFunctionArgs;
                    }

                    break;
                }
            }

            if (eErrorNo != m_eError)
            {
                break;
            }


            //searching trailing "
            bool l_bBackSlash = false;  
            while (l_pItSrc <= i_pArg->pRefStop)
            {
                if ('\\' == *l_pItSrc)
                {
                    l_bBackSlash = true;
                }
                else
                {
                    //todo support special escape sequences like:
                    //\nnn       The byte whose numerical value is given by nnn interpreted as an octal number
                    //\xhh       The byte whose numerical value is given by hh… interpreted as a hexadecimal number
                    //\uhhhh 	 Unicode code point below 10000 hexadecimal
                    //\Uhhhhhhhh Unicode code point where h is a hexadecimal digit 

                    if (l_bBackSlash)
                    {
                        if (    ('\"' == *l_pItSrc)
                             || ('\\' == *l_pItSrc)
                             || ('\'' == *l_pItSrc)
                           )
                        {
                            *l_pItDst = *l_pItSrc; l_pItDst++;
                        }
                        else if ('t' == *l_pItSrc)
                        {
                            *l_pItDst = 9; l_pItDst++;
                        }
                        else if ('n' == *l_pItSrc)
                        {
                            *l_pItDst = 0xA; l_pItDst++;
                        }
                        else if ('r' == *l_pItSrc)
                        {
                            *l_pItDst = 0xD; l_pItDst++;
                        }
                        else if ('v' == *l_pItSrc)
                        {
                            *l_pItDst = 0xB; l_pItDst++;
                        }
                        else if ('?' == *l_pItSrc)
                        {
                            *l_pItDst = '?'; l_pItDst++;
                        }
                        else if ('b' == *l_pItSrc)
                        {
                            *l_pItDst = 8; l_pItDst++;
                        }
                        else 
                        {
                            *l_pItDst = 0x5C;      l_pItDst++;
                            *l_pItDst = *l_pItSrc; l_pItDst++;
                        }
                    }
                    else if ('\"' == *l_pItSrc)
                    {
                        l_pItSrc++;
                        break;
                    }
                    else
                    {
                        *l_pItDst = *l_pItSrc;
                        l_pItDst++;
                    }

                    l_bBackSlash = false;
                }

                l_pItSrc++;
            }
        }

        *l_pItDst = 0;
    }

    return l_pReturn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
char *CFuncRoot::GetParameterRawValue(const stArg *i_pArg)
{
    if (eErrorNo != m_eError)
    {
        return NULL;
    }

    if (!i_pArg)
    {
        m_eError = eErrorFunctionArgs;
        return NULL;
    }

    size_t l_szFormat = i_pArg->pRefStop - i_pArg->pRefStart + 1;
    char  *l_pReturn  = (char*) malloc(l_szFormat + 1);
    if (l_pReturn)
    {
        memcpy(l_pReturn, i_pArg->pRefStart, l_szFormat);
        l_pReturn[l_szFormat] = 0;
    }

    return l_pReturn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t CFuncRoot::GetParameterUintValue(const stArg *i_pArg)
{
    if (eErrorNo != m_eError)
    {
        return 0;
    }

    if (!i_pArg)
    {
        m_eError = eErrorFunctionArgs;
        return 0;
    }


    const char *l_pIt     = i_pArg->pRefStart;
    uint32_t    l_uReturn = 0;
    while (l_pIt <= i_pArg->pRefStop)
    {
        if ((*l_pIt) >= '0' && (*l_pIt) <= '9') 
        {
            l_uReturn *= 10;
            l_uReturn += ((*l_pIt) - '0');
        }
        else
        {
            m_eError = eErrorFunctionArgs;
            break;
        }

        l_pIt++;
    }

    return l_uReturn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
double CFuncRoot::GetParameterDoubleValue(const stArg *i_pArg)
{
    if (eErrorNo != m_eError)
    {
        return 0;
    }

    if (!i_pArg)
    {
        m_eError = eErrorFunctionArgs;
        return 0;
    }

    const char *l_pIt        = i_pArg->pRefStart;
    double      l_dbReturn   = 0;
    double      l_dbReminder = 0;
    double      l_dbDivisor  = 10;
    bool        l_bDot       = false;
    bool        l_bNegative  = false;


    while (l_pIt <= i_pArg->pRefStop)
    {
        if ((*l_pIt) >= '0' && (*l_pIt) <= '9') 
        {
            if (!l_bDot)
            {
                l_dbReturn *= 10.0;
                l_dbReturn += (double)((*l_pIt) - '0');
            }
            else
            {
                l_dbReminder = l_dbReminder+ (double)((*l_pIt) - '0') / l_dbDivisor;
                l_dbDivisor *= 10;
            }
        }
        else if (    ('.' == (*l_pIt))
                  && (!l_bDot)
                )
        {
            l_bDot = true;
        }
        else if ((l_pIt == i_pArg->pRefStart) && ((*l_pIt) == '-')) 
        {
            l_bNegative = true;
        }
        else 
        {
            m_eError = eErrorFunctionArgs;
            break;
        }

        l_pIt++;
    }

    l_dbReturn += l_dbReminder;
    
    if (l_bNegative)
    {
        l_dbReturn = -l_dbReturn;
    }

    return l_dbReturn + l_dbReminder;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CFuncTrace::CFuncTrace(CpreFile *i_pFile, stFuncDesc *i_pDesc, const char *i_pStart, const char *i_pStop, int i_iLine, const char *i_pName)
    : CFuncRoot(i_pFile, i_pDesc, i_pStart, i_pStop, i_iLine, i_pName)
    , m_uId(0)
    , m_pFormat(NULL)
    , m_szBuffer(0)
    , m_pBuffer(NULL)
    , m_pVA(NULL)
    , m_szVA(0)
    , m_szTargetCpuBytes(i_pFile->GetTargetCpuBitsCount() / 8)
    , m_eWchar((eWchar)i_pFile->GetTargetCpuWCharBitsCount())
{
    if (eErrorNo != m_eError)
    {
        return;
    }

    

    m_uId     = GetParameterUintValue(m_cArgs[m_pDesc->pArgs[eTraceIdIndex]]);
    m_pFormat = GetParameterStrValue(m_cArgs[m_pDesc->pArgs[eTraceFormatStringIndex]]);

    ParseFormat();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CFuncTrace::~CFuncTrace()
{
    if (m_pFormat)
    {
        free(m_pFormat);
        m_pFormat = NULL;
    }

    if (m_pBuffer)
    {
        free(m_pBuffer);
        m_pBuffer = NULL;
    }

}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CFuncTrace::SetId(uint16_t i_uId)
{
    if (eErrorNo != m_eError)
    {
        return;
    }

    m_uId = i_uId;

    sP7Trace_Format *l_pHeader_Desc = (sP7Trace_Format *)m_pBuffer;
    l_pHeader_Desc->wID = (tUINT16)m_uId;

    stArg *l_pArg = m_cArgs[m_pDesc->pArgs[eTraceIdIndex]];

    size_t l_szVal = 2; //0 + null terminated

    while (i_uId)
    {
        i_uId /= 10;
        l_szVal ++;
    }

    if (l_pArg->pNewVal)
    {
        free(l_pArg->pNewVal);
        l_pArg->pNewVal = NULL;
    }

    l_pArg->pNewVal = (char*)malloc(l_szVal);
    sprintf(l_pArg->pNewVal, "%d", m_uId);

    m_bUpdated = true;

    m_pFile->SetUpdated(TRUE);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define ADD_ARG(i_bType, i_bSize, oResult)\
{\
    oResult = FALSE;\
    if (l_dwArgsCount < l_dwArgsMax)\
    {\
        l_pArgs[l_dwArgsCount].bType = i_bType;\
        l_pArgs[l_dwArgsCount].bSize = i_bSize;\
        l_dwArgsCount ++;\
        oResult = TRUE;\
    }\
    else if ( l_dwArgsCount >= l_dwArgsMax )\
    {\
        tUINT32       l_dwLength = l_dwArgsCount + 16;\
        sP7Trace_Arg *l_pTMP     = (sP7Trace_Arg*)realloc(l_pArgs, l_dwLength * sizeof(sP7Trace_Arg));\
        if (l_pTMP)\
        {\
            l_pArgs     = l_pTMP;\
            l_dwArgsMax = l_dwLength;\
            l_pArgs[l_dwArgsCount].bType = i_bType;\
            l_pArgs[l_dwArgsCount].bSize = i_bSize;\
            l_dwArgsCount ++;\
            oResult = TRUE;\
        }\
    }\
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum ePrefix_Type
{
    EPREFIX_TYPE_I64   = 0,
    EPREFIX_TYPE_I32      ,
    EPREFIX_TYPE_LL       ,
    EPREFIX_TYPE_L        ,
    EPREFIX_TYPE_HH       ,
    EPREFIX_TYPE_H        ,
    EPREFIX_TYPE_I        ,
    EPREFIX_TYPE_W        ,
    EPREFIX_TYPE_J        ,
    EPREFIX_TYPE_UNKNOWN  
};

struct sPrefix_Desc
{
    const char   *pPrefix;
    tUINT32       dwLen;
    ePrefix_Type  eType;
};


//the order of strings IS VERY important, because we will search for first match 
static const sPrefix_Desc g_pPrefixes[] = { {"I64", 3, EPREFIX_TYPE_I64    }, 
                                            {"I32", 3, EPREFIX_TYPE_I32    },
                                            {"ll",  2, EPREFIX_TYPE_LL     },
                                            {"hh",  2, EPREFIX_TYPE_HH     },
                                            {"h",   1, EPREFIX_TYPE_H      },
                                            {"l",   1, EPREFIX_TYPE_L      },
                                            {"I",   1, EPREFIX_TYPE_I      },
                                            {"z",   1, EPREFIX_TYPE_I      },
                                            {"t",   1, EPREFIX_TYPE_I      },
                                            {"w",   1, EPREFIX_TYPE_W      },
                                            {"j",   1, EPREFIX_TYPE_J      },
                                            {"?",   0, EPREFIX_TYPE_UNKNOWN}
                                          };


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const sPrefix_Desc *Get_Prefix(const char *i_pFormat)
{
    const sPrefix_Desc *l_pPrefix = &g_pPrefixes[0];
    const sPrefix_Desc *l_pReturn = NULL;

    while (l_pPrefix->dwLen)
    {
        if (0 == strncmp(i_pFormat, l_pPrefix->pPrefix, l_pPrefix->dwLen))
        {
            l_pReturn = l_pPrefix;
            break;
        }

        l_pPrefix ++;
    }

    return l_pReturn;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CFuncTrace::ParseFormat()
{
    sP7Trace_Arg  *l_pArgs       = NULL;
    tUINT32        l_dwArgsCount = 0;
    tUINT32        l_dwArgsMax   = 0;
    size_t         l_szFile      = 0;
    size_t         l_szFunc      = 0;
    size_t         l_szFormat    = 0;
    tUINT8         l_bSize       = 0;
    tBOOL          l_bSuccess    = (NULL != m_pFormat);

    if (eErrorNo != m_eError)
    {
        return;
    }


    if (l_bSuccess)
    {
        ePrefix_Type  l_ePrefix   = EPREFIX_TYPE_UNKNOWN;
        const char   *l_pIterator = m_pFormat;
        tBOOL         l_bPercent  = FALSE;

        while (    (*l_pIterator)
                && (l_bSuccess)
              )
        {
            if (FALSE == l_bPercent)
            {
                if ('%' == (*l_pIterator))
                {
                    //we can get "%%" in this case we should ignore "%"
                    l_bPercent = ! l_bPercent;//TRUE;
                    l_ePrefix  = EPREFIX_TYPE_UNKNOWN;
                    l_bSize    = 0;
                }
            }
            else
            {
                switch (*l_pIterator)
                {
                    case '*':
                    {
                        ADD_ARG(P7TRACE_ARG_TYPE_INT32, GetPlatformTypeSize(P7TRACE_ARG_TYPE_INT32), l_bSuccess);
                        l_bSize = uP7_STRING_UNSUPPORTED_SIZE;
                        break;
                    }
                    case 'I':
                    case 'l':
                    case 'h':
                    case 'w':
                    case 'z':
                    case 'j':
                    case 't':
                    {
                        const sPrefix_Desc *l_pPrefix = Get_Prefix(l_pIterator);
                        if (l_pPrefix)
                        {
                            l_ePrefix = l_pPrefix->eType;

                            if (1 < l_pPrefix->dwLen)
                            {
                                l_pIterator += (l_pPrefix->dwLen - 1);
                            }
                        }
                        break;
                    }
                    case 'd':
                    case 'b':
                    case 'i':
                    case 'o':
                    case 'u':
                    case 'x':
                    case 'X':
                    {
                        if (EPREFIX_TYPE_UNKNOWN == l_ePrefix)
                        {
                            //4 bytes integer
                            ADD_ARG(P7TRACE_ARG_TYPE_INT32, GetPlatformTypeSize(P7TRACE_ARG_TYPE_INT32), l_bSuccess);
                        }
                        else if (    (EPREFIX_TYPE_LL  == l_ePrefix)
                                  || (EPREFIX_TYPE_I64 == l_ePrefix)
                                )
                        {
                            //8 bytes integer
                            ADD_ARG(P7TRACE_ARG_TYPE_INT64, GetPlatformTypeSize(P7TRACE_ARG_TYPE_INT64), l_bSuccess);
                        }
                        else if (EPREFIX_TYPE_H == l_ePrefix)
                        {
                            //2 bytes integer
                            ADD_ARG(P7TRACE_ARG_TYPE_INT16, GetPlatformTypeSize(P7TRACE_ARG_TYPE_INT16), l_bSuccess);
                        }
                        else if (EPREFIX_TYPE_HH == l_ePrefix)
                        {
                            //1 bytes integer
                            ADD_ARG(P7TRACE_ARG_TYPE_INT8, GetPlatformTypeSize(P7TRACE_ARG_TYPE_INT8), l_bSuccess);
                        }
                        else if (EPREFIX_TYPE_L == l_ePrefix)
                        {
                            //https://en.cppreference.com/w/cpp/language/types
                            //64 bit systems:
                            //LLP64 or 4/4/8 (int and long are 32-bit, pointer is 64-bit) 
                            //    Win64 API 
                            //LP64 or 4/8/8 (int is 32-bit, long and pointer are 64-bit) 
                            //    Unix and Unix-like systems (Linux, macOS) 
                            //We are targeting here mostly GCC&LLVM compilers and Linux base
                            if (m_szTargetCpuBytes < 8)
                            {
                                ADD_ARG(P7TRACE_ARG_TYPE_INT32, GetPlatformTypeSize(P7TRACE_ARG_TYPE_INT32), l_bSuccess);
                            }
                            else
                            {
                                ADD_ARG(P7TRACE_ARG_TYPE_INT64, GetPlatformTypeSize(P7TRACE_ARG_TYPE_INT64), l_bSuccess);
                            }
                        }
                        else if  (EPREFIX_TYPE_I32 == l_ePrefix)
                        {
                            //4 bytes integer
                            ADD_ARG(P7TRACE_ARG_TYPE_INT32, GetPlatformTypeSize(P7TRACE_ARG_TYPE_INT32), l_bSuccess);
                        }
                        else if (EPREFIX_TYPE_I  == l_ePrefix)
                        {
                            ADD_ARG(P7TRACE_ARG_TYPE_INT32, GetPlatformTypeSize(P7TRACE_ARG_TYPE_INT32), l_bSuccess);
                        }
                        else if (EPREFIX_TYPE_J == l_ePrefix)
                        {
                            //8 bytes integer for most of the platforms, truncate for others
                            ADD_ARG(P7TRACE_ARG_TYPE_INTMAX, GetPlatformTypeSize(P7TRACE_ARG_TYPE_INTMAX), l_bSuccess);
                        }
                        else //by default
                        {
                            //4 bytes integer
                            ADD_ARG(P7TRACE_ARG_TYPE_INT32, GetPlatformTypeSize(P7TRACE_ARG_TYPE_INT32), l_bSuccess);
                        }

                        l_bPercent = FALSE;
                        break;
                    }
                    case 's':
                    case 'S':
                    {
                        if (EPREFIX_TYPE_H == l_ePrefix)
                        {
                            ADD_ARG(P7TRACE_ARG_TYPE_STRA, l_bSize, l_bSuccess); //SIZE_OF_ARG(char*)    
                        }
                        else if (    ('S' == (*l_pIterator))
                                  || (EPREFIX_TYPE_L == l_ePrefix)
                                  || (EPREFIX_TYPE_W == l_ePrefix)
                                )
                        {
                            if (eWchar32 == m_eWchar)
                            {
                                ADD_ARG(P7TRACE_ARG_TYPE_USTR32, l_bSize, l_bSuccess); //SIZE_OF_ARG(char*)    
                            }
                            else
                            {
                                ADD_ARG(P7TRACE_ARG_TYPE_USTR16, l_bSize, l_bSuccess); //SIZE_OF_ARG(char*)    
                            }
                        }
                        else
                        {
                            ADD_ARG(P7TRACE_ARG_TYPE_USTR8, l_bSize, l_bSuccess); //SIZE_OF_ARG(char*)     
                        }

                        l_bPercent = FALSE;
                        break;
                    }
                    //case 'n': ignored, not supported
                    case 'p':
                    {
                        ADD_ARG(P7TRACE_ARG_TYPE_PVOID, GetPlatformTypeSize(P7TRACE_ARG_TYPE_PVOID), l_bSuccess);
                        l_bPercent = FALSE;
                        break;
                    }
                    case 'e':
                    case 'E':
                    case 'f':
                    case 'g':
                    case 'G':
                    case 'a':
                    case 'A':
                    {
                        //8 bytes double
                        ADD_ARG(P7TRACE_ARG_TYPE_DOUBLE, GetPlatformTypeSize(P7TRACE_ARG_TYPE_DOUBLE), l_bSuccess);
                        l_bPercent = FALSE;
                        break;
                    }
                    case 'c':
                    case 'C':
                    {
                        if (EPREFIX_TYPE_H == l_ePrefix)
                        {
                            //1 bytes character
                            ADD_ARG(P7TRACE_ARG_TYPE_CHAR, GetPlatformTypeSize(P7TRACE_ARG_TYPE_CHAR), l_bSuccess);
                        }
                        else if (    (EPREFIX_TYPE_L == l_ePrefix)
                                  || (EPREFIX_TYPE_W == l_ePrefix)
                                )
                        {
                            if (eWchar32 == m_eWchar)
                            {
                                ADD_ARG(P7TRACE_ARG_TYPE_CHAR32, GetPlatformTypeSize(P7TRACE_ARG_TYPE_CHAR32), l_bSuccess); 
                            }
                            else
                            {
                                ADD_ARG(P7TRACE_ARG_TYPE_CHAR16, GetPlatformTypeSize(P7TRACE_ARG_TYPE_CHAR16), l_bSuccess); 
                            }
                        }
                        else if ('c' == (*l_pIterator))
                        {
                            ADD_ARG(P7TRACE_ARG_TYPE_CHAR, GetPlatformTypeSize(P7TRACE_ARG_TYPE_CHAR), l_bSuccess); 
                        }
                        else
                        {
                            //1 bytes character
                            ADD_ARG(P7TRACE_ARG_TYPE_CHAR, GetPlatformTypeSize(P7TRACE_ARG_TYPE_CHAR), l_bSuccess);
                        }

                        l_bPercent = FALSE;
                        break;
                    }
                    case '%':
                    {
                        l_bPercent = FALSE;
                        break;
                    }
                } //switch (*l_pIterator)

                if (    ('0' <= (*l_pIterator))
                     && ('9' >= (*l_pIterator))
                   )
                {
                    l_bSize = uP7_STRING_UNSUPPORTED_SIZE;
                }
            }

            l_pIterator ++;
        } //while (    (*l_pIterator)

        //if (FALSE == m_bInitialized)
        //{
        //    m_dwArgs_Count = 0;
        //    m_pFormat    = "WRONG TRACE ARGUMENTS !";
        //}
    } //if (m_bInitialized)

    if (l_bSuccess)
    {
        m_szBuffer = sizeof(sP7Trace_Format);
                   
        if (l_pArgs)
        {
            m_szBuffer += sizeof(sP7Trace_Arg) * l_dwArgsCount;
        }

        if (m_pFile->GetPath())
        {
            l_szFile += (tUINT32)strlen(m_pFile->GetPath());
        }
        l_szFile ++; //last 0

        if (m_pFunction)
        {
            l_szFunc += (tUINT32)strlen(m_pFunction);
        }
        l_szFunc ++; //last 0

        l_szFormat = (strlen(m_pFormat) + 1)  * sizeof(tWCHAR);
        m_szBuffer     += l_szFormat + l_szFunc + l_szFile;


        m_pBuffer = (tUINT8*)malloc(m_szBuffer);
        if (NULL == m_pBuffer)
        {
            l_bSuccess = FALSE;
        }
    }

    //forming trace format header with all related info
    if (l_bSuccess)
    {
        size_t l_szOffset = 0;

        sP7Trace_Format *l_pHeader_Desc = (sP7Trace_Format *)(m_pBuffer + l_szOffset);

        INIT_EXT_HEADER(l_pHeader_Desc->sCommonRaw, EP7USER_TYPE_TRACE, EP7TRACE_TYPE_DESC, m_szBuffer);
        //l_pHeader_Desc->sCommon.dwSize     = m_dwSize;
        //l_pHeader_Desc->sCommon.dwType     = EP7USER_TYPE_TRACE;
        //l_pHeader_Desc->sCommon.dwSubType  = EP7TRACE_TYPE_DESC;

        l_pHeader_Desc->wArgs_Len          = (tUINT16)l_dwArgsCount;
        l_pHeader_Desc->wID                = (tUINT16)m_uId;
        l_pHeader_Desc->wLine              = (tUINT16)m_iLine;
        l_pHeader_Desc->wModuleID          = (tUINT16)0xFFFF;
        l_szOffset                        += sizeof(sP7Trace_Format);


        if (l_pArgs)
        {
            memcpy(m_pBuffer + l_szOffset, l_pArgs, sizeof(sP7Trace_Arg) * l_dwArgsCount);

            m_pVA  = (sP7Trace_Arg*)(m_pBuffer + l_szOffset);
            m_szVA = l_dwArgsCount;

            l_szOffset += sizeof(sP7Trace_Arg) * l_dwArgsCount;
        }

        //we do not verify address of m_pFormat because we do it before
        Convert_UTF8_To_UTF16(m_pFormat, (tWCHAR*)(m_pBuffer + l_szOffset), (tUINT32)((m_szBuffer - l_szOffset) / sizeof(tWCHAR)));
        l_szOffset += l_szFormat;

        if (m_pFile->GetPath())
        {
            memcpy(m_pBuffer + l_szOffset, m_pFile->GetPath(), l_szFile);
            l_szOffset += l_szFile;
        }
        else
        {
            m_pBuffer[l_szOffset ++] = 0; 
        }

        if (m_pFunction)
        {
            memcpy(m_pBuffer + l_szOffset, m_pFunction, l_szFunc);
            l_szOffset += l_szFunc;
        }
        else
        {
            m_pBuffer[l_szOffset ++] = 0; 
        }
    }

    if (!l_bSuccess)
    {
        m_eError = eErrorTraceFormat;
    }


    if (l_pArgs)
    {
        free(l_pArgs);
        l_pArgs = NULL;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
tUINT8  CFuncTrace::GetPlatformTypeSize(eTrace_Arg_Type i_eType) const
{
    //TODO: store types sizes inside project's XML config file

    if (P7TRACE_ARG_TYPE_CHAR == i_eType)
    {
        return 4;
    }
    else if (P7TRACE_ARG_TYPE_INT8   == i_eType)
    {
        return 4;
    }
    else if (P7TRACE_ARG_TYPE_CHAR16 == i_eType)
    {
        return 4;
    }
    else if (P7TRACE_ARG_TYPE_INT16  == i_eType)
    {
        return 4;
    }
    else if (P7TRACE_ARG_TYPE_INT32  == i_eType)
    {
        return 4;
    }
    else if (P7TRACE_ARG_TYPE_INT64  == i_eType)
    {
        return 8;
    }
    else if (P7TRACE_ARG_TYPE_DOUBLE == i_eType)
    {
        return 8;
    }
    else if (P7TRACE_ARG_TYPE_PVOID  == i_eType)
    {
        return (tUINT8)m_szTargetCpuBytes;
    }
    else if (P7TRACE_ARG_TYPE_USTR16 == i_eType)
    {
        return uP7_STRING_SUPPORTED_SIZE;
    }
    else if (P7TRACE_ARG_TYPE_STRA   == i_eType)
    {
        return uP7_STRING_SUPPORTED_SIZE;
    }
    else if (P7TRACE_ARG_TYPE_USTR8  == i_eType)
    {
        return uP7_STRING_SUPPORTED_SIZE;
    }
    else if (P7TRACE_ARG_TYPE_USTR32 == i_eType)
    {
        return uP7_STRING_SUPPORTED_SIZE;
    }
    else if (P7TRACE_ARG_TYPE_CHAR32 == i_eType)
    {
        return 4;
    }
    else if (P7TRACE_ARG_TYPE_INTMAX == i_eType)
    {
        return (tUINT8)((8 + m_szTargetCpuBytes - 1) & ~(m_szTargetCpuBytes - 1));
    }

    OSPRINT(TM("WARNING: file {%s}:%d Can't recognize argument %d\n"), m_pFile->GetOsPath(), m_iLine, (int32_t)i_eType);
    return 4;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CFuncModule::CFuncModule(CpreFile *i_pFile, stFuncDesc *i_pDesc, const char *i_pStart, const char *i_pStop, int i_iLine, const char *i_pName)
    : CFuncRoot(i_pFile, i_pDesc, i_pStart, i_pStop, i_iLine, i_pName)
    , m_pName(NULL)
    , m_pVerbosity(NULL)
    , m_wId(0)
    , m_uHash(0)
{
    if (eErrorNo != m_eError)
    {
        return;
    }

    m_pName = GetParameterStrValue(m_cArgs[m_pDesc->pArgs[eRegModNameIndex]]);
    m_pVerbosity = GetParameterRawValue(m_cArgs[m_pDesc->pArgs[eRegModLevelIndex]]);
    if (eErrorNo == m_eError)
    {
        m_uHash = _getHash(m_pName);


        INIT_EXT_HEADER(m_sModule.sCommonRaw, EP7USER_TYPE_TRACE, EP7TRACE_TYPE_MODULE, sizeof(sP7Trace_Module));
        //l_pModule->sCommon.dwSize    = sizeof(sP7Trace_Module);
        //l_pModule->sCommon.dwType    = EP7USER_TYPE_TRACE;
        //l_pModule->sCommon.dwSubType = EP7TRACE_TYPE_MODULE;

        m_sModule.eVerbosity        = EP7TRACE_LEVEL_TRACE;
        m_sModule.wModuleID         = 0;

        strncpy(m_sModule.pName, m_pName, P7TRACE_THREAD_NAME_LENGTH);
        m_sModule.pName[P7TRACE_THREAD_NAME_LENGTH - 1] = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CFuncModule::~CFuncModule()
{
    if (m_pName)
    {
        free(m_pName);
        m_pName = NULL;
    }

    if (m_pVerbosity)
    {
        free(m_pVerbosity);
        m_pVerbosity = NULL;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CFuncModule::SetId(tUINT16 i_wId)
{
    if (eErrorNo != m_eError)
    {
        return;
    }

    m_wId = i_wId;
    m_sModule.wModuleID = i_wId;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CFuncCounter::CFuncCounter(CpreFile *i_pFile, stFuncDesc *i_pDesc, const char *i_pStart, const char *i_pStop, int i_iLine, const char *i_pName)
    : CFuncRoot(i_pFile, i_pDesc, i_pStart, i_pStop, i_iLine, i_pName)
    , m_pName(NULL)
    , m_wId(0)
    , m_uHash(0)
    , m_pCounter(NULL)
    , m_szCounter(0)
    , m_dbMin(0.0)
    , m_dbMinAlarm(0.0)
    , m_dbMax(0.0)
    , m_dbMaxAlarm(0.0)
{
    if (eErrorNo != m_eError)
    {
        return;
    }

    m_pName = GetParameterStrValue(m_cArgs[m_pDesc->pArgs[eMkCounterNameIndex]]);
    if (eErrorNo == m_eError)
    {
        m_uHash = _getHash(m_pName);

        size_t l_szCounter = sizeof(sP7Tel_Counter_v2) - P7TELEMETRY_COUNTER_NAME_MIN_LENGTH_V2 * sizeof(tWCHAR);
        size_t l_szName    = strlen(m_pName) + 1;
        l_szCounter += l_szName * sizeof(tWCHAR);

        m_pCounter = (sP7Tel_Counter_v2*)malloc(l_szCounter);

        memset(m_pCounter, 0, l_szCounter);

        m_szCounter = l_szCounter;

        INIT_EXT_HEADER(m_pCounter->sCommonRaw, EP7USER_TYPE_TELEMETRY_V2, EP7TEL_TYPE_COUNTER, l_szCounter);
        //m_sCounter.sCommon.dwSize     = sizeof(sP7Tel_Counter);
        //m_sCounter.sCommon.dwType     = EP7USER_TYPE_TELEMETRY;
        //m_sCounter.sCommon.dwSubType  = EP7TEL_TYPE_COUNTER;

        Convert_UTF8_To_UTF16(m_pName, m_pCounter->pName, (tUINT32)l_szName);
        
        m_pCounter->dbMin = GetParameterDoubleValue(m_cArgs[m_pDesc->pArgs[eMkCounterMinIndex]]);
    }

    if (eErrorNo == m_eError)
    {
        m_pCounter->dbAlarmMin = GetParameterDoubleValue(m_cArgs[m_pDesc->pArgs[eMkCounterAlarmMinIndex]]);
    }

    if (eErrorNo == m_eError)
    {
        m_pCounter->dbMax = GetParameterDoubleValue(m_cArgs[m_pDesc->pArgs[eMkCounterMaxIndex]]);
    }

    if (eErrorNo == m_eError)
    {
        m_pCounter->dbAlarmMax = GetParameterDoubleValue(m_cArgs[m_pDesc->pArgs[eMkCounterAlarmMaxIndex]]);
    }

    if (eErrorNo == m_eError)
    {
        char *l_pOn = GetParameterRawValue(m_cArgs[m_pDesc->pArgs[eMkCounterOnIndex]]);

        if (    (0 == strcmp(l_pOn, "1"))
             || (0 == strcmp(l_pOn, "true"))
             || (0 == strcmp(l_pOn, "TRUE"))
           )
        {
            m_pCounter->bOn = 1;
        }
        else
        {
            m_pCounter->bOn = 0;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CFuncCounter::~CFuncCounter()
{
    if (m_pName)
    {
        free(m_pName);
        m_pName = NULL;
    }

    if (m_pCounter)
    {
        free(m_pCounter);
        m_pCounter = NULL;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CFuncCounter::SetId(tUINT16 i_wId)
{
    if (eErrorNo != m_eError)
    {
        return;
    }

    m_wId           = i_wId;
    m_pCounter->wID = i_wId;
}
