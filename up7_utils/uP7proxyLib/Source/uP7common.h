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
#ifndef UP7_COMMON_H
#define UP7_COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

extern "C" 
{
    #define UP7_FLOATING_POINT //support floating point telemetry, needed for uP7protocol.h
    #include "uP7helpers.h"
    #include "uP7protocol.h"
}

#include "GTypes.h"
#include "Length.h"
#include "AList.h"
#include "RBTree.h"
#include "PString.h"
#include "UTF.h"
#include "WString.h"
#include "PAtomic.h"
#include "PFileSystem.h"
#include "PTime.h"
#include "PProcess.h"
#include "PLock.h"
#include "Lock.h"
#include "PSignal.h"
#include "IMEvent.h"
#include "PMEvent.h"
#include "PThreadShell.h"
#include "PSystem.h"
#include "Ticks.h"
#include "IFile.h"
#include "PFile.h"
#include "PSocket.h"
#include "RingBuffer.h"
#include "P7_Version.h"
#include "P7_Client.h"
#include "P7_Trace.h"
#include "P7_Telemetry.h"
#include "P7_Extensions.h"

#include "uP7proxyApi.h"

#if !defined(SAFE_RELEASE)
    #define SAFE_RELEASE(x) if (x) { x->Release(); x = NULL;}
#endif

#if !defined(BK_LOGGER)
    #define BK_LOGGER m_pP7Trace
#endif

#if !defined(BK_LOG_MODULE)
    #define BK_LOG_MODULE m_hP7Mod
#endif

#if !defined(BK_TELEMETRY)
    #define BK_TELEMETRY m_pP7Tel
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

#ifndef GET_MAX
    #define GET_MAX(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef GET_MIN
    #define GET_MIN(X,Y)            ((X) < (Y) ? (X) : (Y))
#endif

#define uTRACE(i_pFormat,    ...) if (BK_LOGGER) (BK_LOGGER)->P7_TRACE(BK_LOG_MODULE,    i_pFormat, __VA_ARGS__);
#define uDEBUG(i_pFormat,    ...) if (BK_LOGGER) (BK_LOGGER)->P7_DEBUG(BK_LOG_MODULE,    i_pFormat, __VA_ARGS__);
#define uINFO(i_pFormat,     ...) if (BK_LOGGER) (BK_LOGGER)->P7_INFO(BK_LOG_MODULE,     i_pFormat, __VA_ARGS__);
#define uWARNING(i_pFormat,  ...) if (BK_LOGGER) (BK_LOGGER)->P7_WARNING(BK_LOG_MODULE,  i_pFormat, __VA_ARGS__);
#define uERROR(i_pFormat,    ...) if (BK_LOGGER) (BK_LOGGER)->P7_ERROR(BK_LOG_MODULE,    i_pFormat, __VA_ARGS__);
#define uCRITICAL(i_pFormat, ...) if (BK_LOGGER) (BK_LOGGER)->P7_CRITICAL(BK_LOG_MODULE, i_pFormat, __VA_ARGS__);

#define TEL_ADD(i_bCId, i_llValue) if (BK_TELEMETRY) (BK_TELEMETRY)->Add(i_bCId, i_llValue);


#define FIFO_MIN_LENGTH               16384
#define FIFO_MIN_BUFFERS_COUNT        4
#define FIFO_MIN_BUFFER_SIZE          4096
#define FIFO_MAX_BUFFER_SIZE          (128*1024)


#define TIME_SYNC_TIMEOUT_MS              1250
#define uP7_MAINTAIN_TIME_MS              250
#define uP7_MAINTAIN_TIME_STAT_PERIOS_MS  1000
#define COMMAND_TIMEOUT_MS                5000

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
static UNUSED_FUNC tDOUBLE ntohdb(tDOUBLE i_dbX)
{
    tDOUBLE l_dbReturn; //not initialized on purpose
    tUINT8 *l_pSrc = (tUINT8*)(&i_dbX) + 7;
    tUINT8 *l_pDst = (tUINT8*)(&l_dbReturn);

    *l_pDst = *l_pSrc; l_pDst++; l_pSrc--;     //l_pDst[0] = l_pSrc[7];
    *l_pDst = *l_pSrc; l_pDst++; l_pSrc--;     //l_pDst[1] = l_pSrc[6];
    *l_pDst = *l_pSrc; l_pDst++; l_pSrc--;     //l_pDst[2] = l_pSrc[5];
    *l_pDst = *l_pSrc; l_pDst++; l_pSrc--;     //l_pDst[3] = l_pSrc[4];
    *l_pDst = *l_pSrc; l_pDst++; l_pSrc--;     //l_pDst[4] = l_pSrc[3];
    *l_pDst = *l_pSrc; l_pDst++; l_pSrc--;     //l_pDst[5] = l_pSrc[2];
    *l_pDst = *l_pSrc; l_pDst++; l_pSrc--;     //l_pDst[6] = l_pSrc[1];
    *l_pDst = *l_pSrc; l_pDst++; l_pSrc--;     //l_pDst[7] = l_pSrc[0];

    return l_dbReturn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static UNUSED_FUNC float ntohft(float i_ftX)
{
    float   l_ftReturn; //not initialized on purpose
    tUINT8 *l_pSrc = (tUINT8*)(&i_ftX) + 3;
    tUINT8 *l_pDst = (tUINT8*)(&l_ftReturn);

    *l_pDst = *l_pSrc; l_pDst++; l_pSrc--;     //l_pDst[0] = l_pSrc[3];
    *l_pDst = *l_pSrc; l_pDst++; l_pSrc--;     //l_pDst[1] = l_pSrc[2];
    *l_pDst = *l_pSrc; l_pDst++; l_pSrc--;     //l_pDst[2] = l_pSrc[1];
    *l_pDst = *l_pSrc; l_pDst++; l_pSrc--;     //l_pDst[3] = l_pSrc[0];

    return l_ftReturn;
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

#include "uP7chunks.h"
#include "uP7Fifo.h"
#include "uP7proxyLinearPool.h"
#include "uP7proxyPreprocessor.h"

#include "uP7proxyClient.h"
#include "uP7proxyStream.h"

#include "uP7proxyTelemetry.h"
#include "uP7proxyTrace.h"

#include "uP7proxyCpu.h"


#endif //UP7_COMMON_H