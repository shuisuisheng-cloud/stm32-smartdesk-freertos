#include "app_actuator.h"

#include "app_data.h"
#include "main.h"

#define APP_ACTUATOR_RUN_BLINK_MS    1000U
#define APP_ACTUATOR_ALARM_BLINK_MS  200U

static uint32_t led_last_toggle_tick = 0U;

void AppActuator_Init(void)
{
    led_last_toggle_tick = HAL_GetTick();
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
}

void AppActuator_Update(void)
{
    SmartDesk_Data_t *data = AppData_Get();
    uint32_t now = HAL_GetTick();
    uint32_t interval = (data->alarm_on != 0U) ? APP_ACTUATOR_ALARM_BLINK_MS : APP_ACTUATOR_RUN_BLINK_MS;

    if ((now - led_last_toggle_tick) >= interval)
    {
        led_last_toggle_tick = now;
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
    }
}
