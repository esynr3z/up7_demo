////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                     WARNING!                                                       //
//                                       this header is automatically generated                                       //
//                                                 DO NOT MODIFY IT                                                   //
//                                           Generated: 2021.08.12 14:45:29                                           //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef UP7_TARGET_CPU_H
#define UP7_TARGET_CPU_H

uint32_t g_uSessionId = 1876152844;
uint8_t  g_bCrc7 = 15;


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
//uint64_t g_uEpochTime = 0x1d78f6f8c8b7310;
#endif //UP7_TARGET_CPU_H:3269F7B7A4A21A3816FAE1649CCC59CCB05B784DFE1585805C5B1AF3492C58D8