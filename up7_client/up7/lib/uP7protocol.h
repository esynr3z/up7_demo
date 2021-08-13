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
#ifndef UP7PROTOCOL_H
#define UP7PROTOCOL_H

#define uP7_PROTOCOL_ALIGNMENT        4
#define uP7_PROTOCOL_SESSION_UNK      0xFFFFFFFFu
#define uP7_PROTOCOL_SESSION_CRC_UNK  0xFF
#define uP7_MAX_CPU_ID                0xFF
#define uP7_CPU_MAX_COUNT             (((size_t)uP7_MAX_CPU_ID) + 1)
#define uP7_PROTOCOL_MODULE_BITS      13
#define uP7_PROTOCOL_LEVEL_BITS       3
#define up7_SET_TRC_MOD_LVL(MOD, LVL) (MOD | (LVL << uP7_PROTOCOL_MODULE_BITS))
#define up7_GET_TRC_MOD(VAL)          (VAL & ((1 << uP7_PROTOCOL_MODULE_BITS) - 1))
#define up7_GET_TRC_LVL(VAL)          (VAL >> uP7_PROTOCOL_MODULE_BITS)

ST_ASSERT(uP7_PROTOCOL_MODULE_BITS + uP7_PROTOCOL_LEVEL_BITS == 16);

#define uP7_STRING_UNSUPPORTED_SIZE   3
#define uP7_STRING_SUPPORTED_SIZE     0

#define uP7_MAX_PACKET_SIZE           (1 << (sizeof(((struct stuP7baseHdr*)0)->wSize) * 8))
#define uP7_MAX_CHUNKS                ((UP7_MAX_STACK_SIZE_IN_BYTES - UP7_MAX_TRACE_SIZE_IN_BYTES) / sizeof(sizeof(struct stP7packetChunk)))
#define uP7_MAX_TRACE_ARGS            (UP7_MAX_TRACE_SIZE_IN_BYTES - sizeof(struct stuP7traceHdr))
#define uP7_CRC7_MASK                 0x7F
#define uP7_EXT_MASK                  0x80

enum euP7packet
{
    uP7packetTrace = 0,
    uP7packetTraceVerbosiry,
    uP7packetTelemetryDouble,
    uP7packetTelemetryFloat,
    uP7packetTelemetryInt64,
    uP7packetTelemetryInt32,
    uP7packetTelemetryOnOff,
    uP7packetCreateTimeRequest,
    uP7packetCreateTimeResponse,
    uP7packetsMax
};


ST_PACK_ENTER(uP7_PROTOCOL_ALIGNMENT)
struct stuP7baseHdr  
{
    uint32_t uSessionId;
    uint16_t wSize;
    uint8_t  bType;
    uint8_t  bExt1Crc7; //[7]: extension bit, [0..6]: crc7
} ST_ATTR_PACK(uP7_PROTOCOL_ALIGNMENT);

struct stuP7traceHdr 
{
    struct stuP7baseHdr stBase;
    uint64_t qwTime;
    uint32_t uSeqN;
    uint32_t uThreadId;
    uint16_t wId;
    uint16_t bMod13Lvl3; //13 bits for module, 3 bits for level, no bit fields :)
    //At the end of structure there is serialized data:
    // - trace variable arguments values
    // - extensions [data X bits][type 8 bits], [data X bits][type 8 bits], ... [count 8 bits]
} ST_ATTR_PACK(uP7_PROTOCOL_ALIGNMENT);

struct stuP7traceVerbosityHdr 
{
    struct stuP7baseHdr stBase;
    uint16_t wModuleId; 
    uint8_t  bLevel; 
} ST_ATTR_PACK(uP7_PROTOCOL_ALIGNMENT);


enum euP7telVal //flags for telemetry sample > stuP7baseHdr::bFlags
{
    uP7telValDouble = 0,
    uP7telValFloat  = 1,
    uP7telValInt64  = 2,
    uP7telValInt32  = 3,
    uP7telValMask   = 0x3
};


#if defined(UP7_FLOATING_POINT)
struct stuP7telF8Hdr //telemetry sample float 8 bytes.
{
    struct stuP7baseHdr stBase;
    uint64_t         qwTime;
    double           tValue;
    uint16_t         wId;
} ST_ATTR_PACK(uP7_PROTOCOL_ALIGNMENT);

struct stuP7telF4Hdr //telemetry sample float 4 bytes. 
{
    struct stuP7baseHdr stBase;
    uint64_t         qwTime;
    float            tValue;
    uint16_t         wId;
} ST_ATTR_PACK(uP7_PROTOCOL_ALIGNMENT);
#endif

struct stuP7telI8Hdr //telemetry sample int 8 bytes.
{
    struct stuP7baseHdr stBase;
    uint64_t         qwTime;
    int64_t          tValue;
    uint16_t         wId;
} ST_ATTR_PACK(uP7_PROTOCOL_ALIGNMENT);

struct stuP7telI4Hdr //telemetry sample int 4 bytes.
{
    struct stuP7baseHdr stBase;
    uint64_t         qwTime;
    int32_t          tValue;
    uint16_t         wId;
} ST_ATTR_PACK(uP7_PROTOCOL_ALIGNMENT);


struct stuP7telOnOffHdr 
{
    struct stuP7baseHdr stBase;
    uint16_t         wId;
    uint8_t          bOn;
} ST_ATTR_PACK(uP7_PROTOCOL_ALIGNMENT);

struct stuP7timeReqHdr 
{
    struct stuP7baseHdr stBase;
} ST_ATTR_PACK(uP7_PROTOCOL_ALIGNMENT);

struct stuP7timeResHdr 
{
    struct stuP7baseHdr stBase;
    uint64_t            qwCpuFreq;
    uint64_t            qwCpuStartTime;
    uint64_t            qwCpuCurrenTime;
} ST_ATTR_PACK(uP7_PROTOCOL_ALIGNMENT);


ST_PACK_EXIT()


#endif //UP7PROTOCOL_H
