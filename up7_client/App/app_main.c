#include "app_main.h"
#include "math.h"
#include "string.h"
#include "telem.h"

// based on TIM9 timer to emulate asynchronous events
volatile bool toogle_led = false;
volatile bool inc_ramp = false;
volatile bool calc_sin = false;
volatile bool send_alive = false;
void app_tick_handler(void)
{
    static int tick_counter;
    tick_counter++;
    if (tick_counter % 250 == 0) { // 250ms
        toogle_led = true;
    }
    if (tick_counter % 10 == 0) { // 10ms
        inc_ramp = true;
    }
    if (tick_counter % 5000 == 0) { // 5s
        send_alive = true;
    }
    // every 1ms
    calc_sin = true;
}

void app_main(void)
{
    // init up7 lib
    telem_init();

    // register telemetry counters
    static huP7TelId led_id = uP7_TELEMETRY_INVALID_ID;
    if (!uP7TelCreateCounter("led", 0, 0, 1, 1, true, &led_id)) {
        Error_Handler();
    }
    static huP7TelId ramp_id = uP7_TELEMETRY_INVALID_ID;
    if (!uP7TelCreateCounter("ramp", -10, -10, 10, 10, true, &ramp_id)) {
        Error_Handler();
    }
    static huP7TelId sin_id = uP7_TELEMETRY_INVALID_ID;
    if (!uP7TelCreateCounter("sin", -256, -256, 256, 256, true, &sin_id)) {
        Error_Handler();
    }
    // register telemetry modules
    static huP7Module dbg_id = uP7_MODULE_INVALID_ID;
    if (!uP7TrcRegisterModule("dbg", euP7Level_Trace, &dbg_id)) {
        Error_Handler();
    }

    HAL_Delay(5000);
    uP7INF(1, dbg_id, "Hello from the STM32F411 running at %d MHz!", SystemCoreClock / 1000000);
    HAL_Delay(100);
    uP7TRC(2, dbg_id, "Some trace message.");
    HAL_Delay(10);
    uP7DBG(3, dbg_id, "Debug this thing...");
    HAL_Delay(20);
    uP7INF(4, dbg_id, "Some usefull information special for you.");
    HAL_Delay(40);
    uP7WRN(5, dbg_id, "Things get worse.");
    HAL_Delay(30);
    uP7ERR(6, dbg_id, "Really bad things happen!");
    HAL_Delay(50);
    uP7CRT(7, dbg_id, "Ragnarok!");

    // start timer
    if (HAL_TIM_Base_Start_IT(&htim9) != HAL_OK) {
        Error_Handler();
    }
    int32_t ramp_val = -10;
    int32_t sin_arg = 0;
    int32_t sin_val = 0;
    while (1) {
        if (toogle_led) {
            toogle_led = false;
            HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
            uP7TelSentSample(led_id, (int32_t)HAL_GPIO_ReadPin(LED_GPIO_Port, LED_Pin));
        }
        if (inc_ramp) {
            inc_ramp = false;
            uP7TelSentSample(ramp_id, ramp_val);
            if (ramp_val == 10)
                ramp_val = -10;
            else
                ramp_val++;
        }
        if (calc_sin) {
            calc_sin = false;
            uP7TelSentSample(sin_id, sin_val);
            if (sin_arg == 256)
                sin_arg = 0;
            else
                sin_arg++;
            sin_val = (int32_t)(256 * sinf(6.28 * (float)sin_arg / 256.0));
        }
        if (send_alive) {
            send_alive = false;
            uP7TRC(8, dbg_id, "Yep, I'm still alive.");
        }
    };
}
