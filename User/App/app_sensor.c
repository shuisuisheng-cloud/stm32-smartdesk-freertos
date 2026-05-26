#include "app_sensor.h"

#include "app_clock.h"
#include "app_data.h"
#include "main.h"

extern ADC_HandleTypeDef hadc1;

static uint16_t AppSensor_ReadGasSimRaw(void);
static uint8_t AppSensor_GasRawToPercent(uint16_t raw);

void AppSensor_Init(void)
{
    AppSensor_UpdateData();
}

uint16_t AppSensor_ReadLightAdc(void)
{
    uint16_t value = 0U;

    if (HAL_ADC_Start(&hadc1) == HAL_OK)
    {
        if (HAL_ADC_PollForConversion(&hadc1, 10U) == HAL_OK)
        {
            value = (uint16_t)HAL_ADC_GetValue(&hadc1);
        }

        HAL_ADC_Stop(&hadc1);
    }

    return value;
}

uint16_t AppSensor_ReadGasRaw(void)
{
#if APP_GAS_USE_ADC
    /*
     * TODO: Select the MQ gas ADC channel before conversion when hardware is wired.
     * Current ADC1 use is reserved for PA1 light input, so do not bind gas ADC here yet.
     */
    return 0U;
#else
    return AppSensor_ReadGasSimRaw();
#endif
}

void AppSensor_UpdateData(void)
{
    SmartDesk_Data_t *data = AppData_Get();
    uint8_t gas_alarm = 0U;

    data->light_adc = AppSensor_ReadLightAdc();
    data->gas_raw = AppSensor_ReadGasRaw();
    data->gas_adc = data->gas_raw;
    data->gas_percent = AppSensor_GasRawToPercent(data->gas_raw);

#if APP_GAS_USE_DO
    /*
     * TODO: Replace GPIOx/GPIO_PIN_x with the MQ DO pin when the digital output is wired.
     * gas_alarm = (HAL_GPIO_ReadPin(GPIOx, GPIO_PIN_x) == APP_GAS_DO_ACTIVE_LEVEL) ? 1U : 0U;
     */
    gas_alarm = 0U;
#elif APP_GAS_SIM_ALARM_TEST
    gas_alarm = 1U;
#else
    gas_alarm = (data->gas_raw >= APP_GAS_ALARM_THRESHOLD_RAW) ? 1U : 0U;
#endif

    data->gas_alarm = gas_alarm;
    data->alarm_on = ((gas_alarm != 0U) || (AppClock_IsAlarmTriggered() != 0U)) ? 1U : 0U;

    data->comfort_score = AppData_CalcComfortScore(data);
}

static uint16_t AppSensor_ReadGasSimRaw(void)
{
    static uint16_t gas_raw = 300U;
    static int16_t step = 10;
    static uint32_t last_update_tick = 0U;

    if ((HAL_GetTick() - last_update_tick) < 1000U)
    {
        return gas_raw;
    }

    last_update_tick = HAL_GetTick();

    gas_raw = (uint16_t)(gas_raw + step);
    if (gas_raw >= 900U)
    {
        gas_raw = 900U;
        step = -10;
    }
    else if (gas_raw <= 300U)
    {
        gas_raw = 300U;
        step = 10;
    }

    return gas_raw;
}

static uint8_t AppSensor_GasRawToPercent(uint16_t raw)
{
    uint32_t percent;

    if (raw >= 4095U)
    {
        return 100U;
    }

    percent = ((uint32_t)raw * 100UL) / 4095UL;
    return (uint8_t)percent;
}
