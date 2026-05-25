#include "app_key.h"

#include "main.h"

#define APP_KEY_DEBOUNCE_MS    30U
#define APP_KEY_LONG_PRESS_MS  1000U
#define APP_KEY_PRESSED_LEVEL  GPIO_PIN_RESET

static GPIO_PinState key_last_raw_state = GPIO_PIN_SET;
static GPIO_PinState key_stable_state = GPIO_PIN_SET;
static uint32_t key_last_change_tick = 0U;
static uint32_t key_press_start_tick = 0U;

void AppKey_Init(void)
{
    key_last_raw_state = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);
    key_stable_state = key_last_raw_state;
    key_last_change_tick = HAL_GetTick();
    key_press_start_tick = 0U;
}

uint8_t AppKey_Scan(void)
{
    GPIO_PinState raw_state = HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin);
    uint32_t now = HAL_GetTick();

    if (raw_state != key_last_raw_state)
    {
        key_last_raw_state = raw_state;
        key_last_change_tick = now;
    }

    if ((now - key_last_change_tick) < APP_KEY_DEBOUNCE_MS)
    {
        return APP_KEY_EVENT_NONE;
    }

    if (raw_state != key_stable_state)
    {
        key_stable_state = raw_state;

        if (key_stable_state == APP_KEY_PRESSED_LEVEL)
        {
            key_press_start_tick = now;
        }
        else
        {
            if ((now - key_press_start_tick) >= APP_KEY_LONG_PRESS_MS)
            {
                return APP_KEY_EVENT_LONG;
            }

            return APP_KEY_EVENT_SHORT;
        }
    }

    return APP_KEY_EVENT_NONE;
}
