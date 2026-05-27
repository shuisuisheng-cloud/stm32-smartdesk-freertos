#include "app_actuator.h"

#include "app_data.h"
#include "main.h"

#define APP_ACTUATOR_RUN_BLINK_MS    1000U
#define APP_ACTUATOR_ALARM_BLINK_MS  200U
#define APP_ACTUATOR_BUZZER_ON_MS    100U
#define APP_ACTUATOR_BUZZER_OFF_MS   900U

static uint32_t led_last_toggle_tick = 0U;
static uint32_t buzzer_last_change_tick = 0U;

void AppActuator_Init(void)
{
    SmartDesk_Data_t *data = AppData_Get();

    led_last_toggle_tick = HAL_GetTick();
    buzzer_last_change_tick = HAL_GetTick();
    data->fan_enabled = APP_FAN_ENABLE ? 1U : 0U;
    data->fan_on = 0U;
    data->fan_speed_percent = 0U;
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    AppActuator_BuzzerOff();
    AppActuator_FanOff();
}

void AppActuator_Update(void)
{
    SmartDesk_Data_t *data = AppData_Get();
    uint32_t now = HAL_GetTick();
    uint32_t interval = (data->alarm_on != 0U) ? APP_ACTUATOR_ALARM_BLINK_MS : APP_ACTUATOR_RUN_BLINK_MS;
    uint32_t buzzer_interval;

    if ((now - led_last_toggle_tick) >= interval)
    {
        led_last_toggle_tick = now;
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
    }

    if (data->gas_alarm != 0U)
    {
        buzzer_interval = (data->buzzer_on != 0U) ? APP_ACTUATOR_BUZZER_ON_MS : APP_ACTUATOR_BUZZER_OFF_MS;
        if ((now - buzzer_last_change_tick) >= buzzer_interval)
        {
            buzzer_last_change_tick = now;
            AppActuator_BuzzerToggle();
        }
    }
    else
    {
        buzzer_last_change_tick = now;
        AppActuator_BuzzerOff();
    }

    if (data->fan_on != 0U)
    {
        AppActuator_FanOn();
    }
    else
    {
        AppActuator_FanOff();
    }
}

void AppActuator_BuzzerOn(void)
{
    AppData_Get()->buzzer_on = 1U;
#if APP_BUZZER_ENABLE
    HAL_GPIO_WritePin(APP_BUZZER_GPIO_PORT, APP_BUZZER_GPIO_PIN, APP_BUZZER_ACTIVE_LEVEL);
#endif
}

void AppActuator_BuzzerOff(void)
{
    AppData_Get()->buzzer_on = 0U;
#if APP_BUZZER_ENABLE
    HAL_GPIO_WritePin(APP_BUZZER_GPIO_PORT,
                      APP_BUZZER_GPIO_PIN,
                      (APP_BUZZER_ACTIVE_LEVEL == GPIO_PIN_SET) ? GPIO_PIN_RESET : GPIO_PIN_SET);
#endif
}

void AppActuator_BuzzerToggle(void)
{
    if (AppData_Get()->buzzer_on != 0U)
    {
        AppActuator_BuzzerOff();
    }
    else
    {
        AppActuator_BuzzerOn();
    }
}

void AppActuator_FanOn(void)
{
    SmartDesk_Data_t *data = AppData_Get();

    data->fan_enabled = APP_FAN_ENABLE ? 1U : 0U;
    data->fan_on = 1U;
    if (data->fan_speed_percent == 0U)
    {
        data->fan_speed_percent = 100U;
    }

#if APP_FAN_ENABLE
#if APP_FAN_USE_PWM
    /*
     * TODO: Set PWM duty cycle after the fan driver TIM channel is configured.
     */
#else
    HAL_GPIO_WritePin(APP_FAN_GPIO_PORT, APP_FAN_GPIO_PIN, APP_FAN_ACTIVE_LEVEL);
#endif
#endif
}

void AppActuator_FanOff(void)
{
    SmartDesk_Data_t *data = AppData_Get();

    data->fan_enabled = APP_FAN_ENABLE ? 1U : 0U;
    data->fan_on = 0U;
    data->fan_speed_percent = 0U;

#if APP_FAN_ENABLE
#if APP_FAN_USE_PWM
    /*
     * TODO: Set PWM duty cycle to 0 after the fan driver TIM channel is configured.
     */
#else
    HAL_GPIO_WritePin(APP_FAN_GPIO_PORT,
                      APP_FAN_GPIO_PIN,
                      (APP_FAN_ACTIVE_LEVEL == GPIO_PIN_SET) ? GPIO_PIN_RESET : GPIO_PIN_SET);
#endif
#endif
}

void AppActuator_FanSetSpeed(uint8_t percent)
{
    SmartDesk_Data_t *data = AppData_Get();

    if (percent > 100U)
    {
        percent = 100U;
    }

    data->fan_enabled = APP_FAN_ENABLE ? 1U : 0U;
    data->fan_speed_percent = percent;
    data->fan_on = (percent != 0U) ? 1U : 0U;

#if APP_FAN_ENABLE
#if APP_FAN_USE_PWM
    /*
     * TODO: Convert percent to PWM duty cycle after the TIM channel is configured.
     */
#else
    HAL_GPIO_WritePin(APP_FAN_GPIO_PORT,
                      APP_FAN_GPIO_PIN,
                      (percent != 0U) ? APP_FAN_ACTIVE_LEVEL :
                      ((APP_FAN_ACTIVE_LEVEL == GPIO_PIN_SET) ? GPIO_PIN_RESET : GPIO_PIN_SET));
#endif
#endif
}

void AppActuator_FanToggle(void)
{
    if (AppData_Get()->fan_on != 0U)
    {
        AppActuator_FanOff();
    }
    else
    {
        AppActuator_FanOn();
    }
}
