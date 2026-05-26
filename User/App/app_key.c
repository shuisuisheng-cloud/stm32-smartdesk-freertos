#include "app_key.h"

#include "main.h"

#include <stdio.h>

#define APP_KEY_SHORT_MIN_MS      30U
#define APP_KEY_LONG_PRESS_MS     2500U
#define APP_KEY_EVENT_GUARD_MS    150U

static GPIO_PinState last_raw = GPIO_PIN_SET;
static uint8_t pressed = 0U;
static uint32_t press_start_tick = 0U;
static uint32_t last_event_tick = 0U;

void AppKey_Init(void)
{
    last_raw = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);
    pressed = 0U;
    press_start_tick = 0U;
    last_event_tick = 0U;
}

uint8_t AppKey_Scan(void)
{
    GPIO_PinState raw = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);
    uint32_t now = HAL_GetTick();
    uint32_t duration;

    if (raw != last_raw)
    {
        if ((last_raw == GPIO_PIN_SET) && (raw == GPIO_PIN_RESET))
        {
            if (pressed == 0U)
            {
                press_start_tick = now;
                pressed = 1U;
            }
        }
        else if ((last_raw == GPIO_PIN_RESET) && (raw == GPIO_PIN_SET))
        {
            if (pressed != 0U)
            {
                duration = now - press_start_tick;
                pressed = 0U;
                press_start_tick = 0U;

                if ((now - last_event_tick) < APP_KEY_EVENT_GUARD_MS)
                {
                    last_raw = raw;
                    return APP_KEY_EVENT_NONE;
                }

                if (duration >= APP_KEY_LONG_PRESS_MS)
                {
                    last_event_tick = now;
                    last_raw = raw;
                    printf("[KEY] long\r\n");
                    return APP_KEY_EVENT_LONG;
                }

                if (duration >= APP_KEY_SHORT_MIN_MS)
                {
                    last_event_tick = now;
                    last_raw = raw;
                    printf("[KEY] short\r\n");
                    return APP_KEY_EVENT_SHORT;
                }
            }
        }

        last_raw = raw;
    }

    return APP_KEY_EVENT_NONE;
}
