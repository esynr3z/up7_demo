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
#ifndef UP7_PRE_DEFINES_H
#define UP7_PRE_DEFINES_H

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum eErrorCodes
{
    eErrorNo                  = 0,
    eErrorArguments              ,
    eErrorXmlParsing             ,
    eErrorFileOpen               ,
    eErrorFileTooBig             ,
    eErrorFileRead               ,
    eErrorFunctionArgs           ,
    eErrorNotFunction            ,
    eErrorTraceId                ,
    eErrorTraceFormat            ,
    eErrorMemAlloc               ,
    eErrorTraceIdOverFlow        ,
    eErrorTraceIdDuplicate       ,
    eErrorFileWrite              ,
    eErrorNameDuplicated         ,
    eErrorMaxValue
};


#if !defined(TIME_OFFSET_1601_1970)
    //time offset from January 1, 1601 to January 1, 1970, resolution 100ns
    #define TIME_OFFSET_1601_1970                            (116444736000000000ULL)
#endif


#if defined(_WIN32) || defined(_WIN64)
    #define OSPRINT(i_pFormat, ...) wprintf_s(i_pFormat, __VA_ARGS__)
    #define OSSTRDUP(i_pStr) _strdup(i_pStr)
#elif defined(__linux__)
    #define OSPRINT(i_pFormat, ...) printf(i_pFormat, __VA_ARGS__)
    #define OSSTRDUP(i_pStr) strdup(i_pStr)
#endif


#if !defined(SAFE_RELEASE)
    #define SAFE_RELEASE(x) if (x) { x->Release(); x = NULL;}
#endif

#if !defined(MAXINT16)
    #define MAXINT16 0xFFFF
#endif

#if !defined(MAXINT32)
    #define MAXINT32 2147483647
#endif

#if !defined(MAXUINT32)
    #define MAXUINT32 0xFFFFFFFF
#endif

#if !defined(MAXUINT64)
    #define MAXUINT64 0xFFFFFFFFFFFFFFFFull
    #define MAXINT64    ((tINT64)(MAXUINT64 >> 1))
    #define MININT64    ((tINT64)~MAXINT64)
#endif

#if !defined(UINTMAX_MAX)
    typedef uint64_t uintmax_t;
    #define UINTMAX_MAX	0xffffffffffffffffU
#endif

#if !defined(INTMAX_MAX)
    typedef int64_t intmax_t;
     #define INTMAX_MIN		(-0x7fffffffffffffff - 1)
     #define INTMAX_MAX		0x7fffffffffffffff
#endif

#ifndef GET_MAX
    #define GET_MAX(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef GET_MIN
    #define GET_MIN(X,Y)            ((X) < (Y) ? (X) : (Y))
#endif

#define P7TRACE_ITEM_BLOCK_ASTRING                                          (-1)
#define P7TRACE_ITEM_BLOCK_USTRING16                                        (-2)
#define P7TRACE_ITEM_BLOCK_USTRING8                                         (-3)
#define P7TRACE_ITEM_BLOCK_USTRING32                                        (-4)
#define P7TRACE_ITEM_BLOCK_ASTRING_FIX                                      (-5)
#define P7TRACE_ITEM_BLOCK_WSTRING_FIX                                      (-6)


#define XML_NODE_MAIN                         TM("uP7preProcessor")
#define XML_NODE_OPTIONS                        TM("Options")
#define XML_NODE_OPTIONS_PROJECT                  TM("Project")
#define XML_ATTE_OPTIONS_PROJECT_NAME                TM("Name")
#define XML_ATTE_OPTIONS_PROJECT_BITS                TM("Bits")
#define XML_ATTE_OPTIONS_PROJECT_WCHAR               TM("wchar_t")
#define XML_ATTE_OPTIONS_PROJECT_IDS                 TM("IDsHeader")

#define XML_NODE_OPTIONS_PROCESS                  TM("Process")
#define XML_NODE_OPTIONS_PROCESS_PATTERN            TM("Pattern")
#define XML_NARG_OPTIONS_PROCESS_MASK                 TM("Mask")

#define XML_NODE_OPTIONS_DESC                     TM("Descriptors")
#define XML_NODE_OPTIONS_DESC_MODULE                TM("Module")
#define XML_NODE_OPTIONS_DESC_MODULE_NAME             TM("Name")
#define XML_NODE_OPTIONS_DESC_MODULE_VERB             TM("Verbosity")
#define XML_NODE_OPTIONS_DESC_COUNTER               TM("Counter")
#define XML_NODE_OPTIONS_DESC_COUNTER_NAME            TM("Name")
#define XML_NODE_OPTIONS_DESC_COUNTER_MIN             TM("Min")
#define XML_NODE_OPTIONS_DESC_COUNTER_ALARM_MIN       TM("AlarmMin")
#define XML_NODE_OPTIONS_DESC_COUNTER_MAX             TM("Max")
#define XML_NODE_OPTIONS_DESC_COUNTER_ALARM_MAX       TM("AlarmMax")
#define XML_NODE_OPTIONS_DESC_COUNTER_ENABLED         TM("Enabled")

#define XML_NODE_OPTIONS_READONLY                 TM("ReadOnlyFiles")
#define XML_NODE_OPTIONS_READONLY_FILE              TM("File")
#define XML_NARG_OPTIONS_READONLY_RALTIVE_PATH      TM("RelativePath")

#define XML_NODE_OPTIONS_EXCLUDED                 TM("ExcludedFiles")
#define XML_NODE_OPTIONS_EXCLUDED_FILE              TM("File")
#define XML_NARG_OPTIONS_EXCLUDED_RALTIVE_PATH      TM("RelativePath")
                                              
#define XML_NODE_OPTIONS_FUNC                   TM("Functions")
#define XML_NARG_OPTIONS_FUNC_NAME                  TM("Name")
#define XML_NODE_OPTIONS_FUNC_TRACE               TM("Trace")
#define XML_NARG_OPTIONS_FUNC_TRACE_ID              TM("IdIndex")
#define XML_NARG_OPTIONS_FUNC_TRACE_FORMAT          TM("FormatStrIndex")
#define XML_NODE_OPTIONS_FUNC_REGMOD              TM("RegisterModule")
#define XML_NARG_OPTIONS_FUNC_REGMOD_NAME           TM("NameIndex")
#define XML_NARG_OPTIONS_FUNC_REGMOD_LEVEL          TM("LevelIndex")
#define XML_NODE_OPTIONS_FUNC_MKCOUNTER           TM("CreateCounter")
#define XML_NARG_OPTIONS_FUNC_MKCOUNTER_NAME        TM("NameIndex")
#define XML_NARG_OPTIONS_FUNC_MKCOUNTER_MIN         TM("MinIndex")
#define XML_NARG_OPTIONS_FUNC_MKCOUNTER_AMIN        TM("AlarmMinIndex")
#define XML_NARG_OPTIONS_FUNC_MKCOUNTER_MAX         TM("MaxIndex")
#define XML_NARG_OPTIONS_FUNC_MKCOUNTER_AMAX        TM("AlarmMaxIndex")
#define XML_NARG_OPTIONS_FUNC_MKCOUNTER_ON          TM("OnIndex")

#define XML_NODE_FILES                          TM("Files")
#define XML_NODE_FILES_ITEM                       TM("File")
#define XML_NARG_FILES_ITEM_RELATIVE_PATH           TM("RelativePath")
#define XML_NARG_FILES_ITEM_HASH                    TM("Hash")
                                               
#define CFG_FILE_INDEX                           1
#define DIR_SRC_INDEX                            2
#define DIR_OUT_INDEX                            3


#endif //UP7_PRE_DEFINES_H