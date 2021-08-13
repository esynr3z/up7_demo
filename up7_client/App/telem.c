#include "telem.h"
#include "uP7IDs.h"
#include "uP7preprocessed.h"
#include "usbd_cdc_if.h"

uint64_t get_timer_frequency(void* i_pCtx)
{
    UNUSED_ARG(i_pCtx);
    return 1000 / HAL_GetTickFreq();
}

uint64_t get_timer_value(void* i_pCtx)
{
    UNUSED_ARG(i_pCtx);
    return HAL_GetTick();
}

#define TXBUF_SIZE 2048
static uint8_t txbuf[TXBUF_SIZE];
static size_t txbuf_wptr = 0;

bool send_telem_packet(void* i_pCtx, enum euP7packetRank i_eRank,
                       const struct stP7packetChunk* i_pChunks,
                       size_t i_szChunks, size_t i_szData)
{
    UNUSED_ARG(i_pCtx);
    UNUSED_ARG(i_eRank);
    UNUSED_ARG(i_szData);
    while (i_szChunks--) {
        memcpy(&txbuf[txbuf_wptr], (uint8_t*)i_pChunks->pData, i_pChunks->szData);
        txbuf_wptr += i_pChunks->szData;
        i_pChunks++;
    }
    if (txbuf_wptr >= TXBUF_SIZE / 2) {
        // NOTE: this way of sending might be a problem if telemetry messages are rare
        CDC_Transmit_FS(txbuf, txbuf_wptr);
        txbuf_wptr = 0;
    }

    return true;
}

void telem_init(void)
{
    struct stuP7config l_stConfig;

    l_stConfig.bSessionIdCrc7 = g_bCrc7;
    l_stConfig.uSessionId = g_uSessionId;
    l_stConfig.pCtxTimer = NULL;
    l_stConfig.cbTimerFrequency = get_timer_frequency;
    l_stConfig.cbTimerValue = get_timer_value;
    l_stConfig.pCtxLock = NULL;
    l_stConfig.cbLock = NULL;
    l_stConfig.cbUnLock = NULL;
    l_stConfig.pCtxPacket = NULL;
    l_stConfig.cbSendPacket = send_telem_packet;
    l_stConfig.pCtxThread = NULL;
    l_stConfig.cbGetCurrentThreadId = NULL;
    l_stConfig.eDefaultVerbosity = euP7Level_Trace;

    l_stConfig.pModules = g_pModules;
    l_stConfig.szModules = g_szModules;
    l_stConfig.pTraces = g_pTraces;
    l_stConfig.szTraces = g_szTraces;
    l_stConfig.pTelemetry = g_pTelemetry;
    l_stConfig.szTelemetry = g_szTelemetry;
    l_stConfig.eAlignment = euP7alignment4;

    uP7initialize(&l_stConfig);
}
