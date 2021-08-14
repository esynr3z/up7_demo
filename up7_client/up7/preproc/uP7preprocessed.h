////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                     WARNING!                                                       //
//                                       this header is automatically generated                                       //
//                                                 DO NOT MODIFY IT                                                   //
//                                           Generated: 2021.08.13 19:18:39                                           //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef UP7_TARGET_CPU_H
#define UP7_TARGET_CPU_H

uint32_t g_uSessionId = 1750910093;
uint8_t  g_bCrc7 = 36;


size_t g_szModules = 2;
struct stuP7Module g_pModules[] = 
{
    {"XML Module", euP7Level_Trace, 0, 234093310},
    {"dbg", euP7Level_Trace, 1, 3326915628}
};

size_t g_szTelemetry = 4;
struct stuP7telemetry g_pTelemetry[] = 
{
    {"XML Counter", 622366530, true, 0},
    {"led", 1080748746, true, 1},
    {"ramp", 2409181673, true, 2},
    {"sin", 3761252941, true, 3}
};

static const struct stuP7arg g_pArgsId1[] = { {(uint8_t)euP7_arg_int32, 4} };

size_t g_szTraces = 8;
struct stuP7Trace g_pTraces[] = 
{
    {1, sizeof(g_pArgsId1)/sizeof(struct stuP7arg), g_pArgsId1},
    {2, 0, NULL},
    {3, 0, NULL},
    {4, 0, NULL},
    {5, 0, NULL},
    {6, 0, NULL},
    {7, 0, NULL},
    {8, 0, NULL}
};
//uint64_t g_uEpochTime = 0x1d7905ee0532160;
#endif //UP7_TARGET_CPU_H:D396C4826B79131AF0FC7D4368E759AFDB32C13BC895B6D0036E87BB0ACE285B