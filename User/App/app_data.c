#include "app_data.h"

static SmartDesk_Data_t smartdesk_data;

void AppData_Init(void)
{
    AppData_UpdateFake();
}

SmartDesk_Data_t* AppData_Get(void)
{
    return &smartdesk_data;
}

void AppData_UpdateFake(void)
{
    smartdesk_data.temperature = 26.0f;
    smartdesk_data.humidity = 55.0f;
    smartdesk_data.gas_adc = 800U;
    smartdesk_data.light_adc = 1200U;
    smartdesk_data.fan_on = 0U;
    smartdesk_data.light_on = 0U;
    smartdesk_data.alarm_on = 0U;
    smartdesk_data.mode = 0U;
    smartdesk_data.comfort_score = AppData_CalcComfortScore(&smartdesk_data);
}

uint8_t AppData_CalcComfortScore(SmartDesk_Data_t *data)
{
    int16_t score = 100;

    if (data == 0)
    {
        return 0U;
    }

    if (data->temperature < 22.0f)
    {
        score -= (int16_t)((22.0f - data->temperature) * 3.0f);
    }
    else if (data->temperature > 28.0f)
    {
        score -= (int16_t)((data->temperature - 28.0f) * 3.0f);
    }

    if (data->humidity < 40.0f)
    {
        score -= (int16_t)((40.0f - data->humidity) * 2.0f);
    }
    else if (data->humidity > 65.0f)
    {
        score -= (int16_t)((data->humidity - 65.0f) * 2.0f);
    }

    if (data->gas_adc > 1000U)
    {
        score -= (int16_t)((data->gas_adc - 1000U) / 30U);
    }

    if (data->light_adc < 500U)
    {
        score -= (int16_t)((500U - data->light_adc) / 20U);
    }
    else if (data->light_adc > 2500U)
    {
        score -= (int16_t)((data->light_adc - 2500U) / 30U);
    }

    if (score < 0)
    {
        score = 0;
    }
    else if (score > 100)
    {
        score = 100;
    }

    return (uint8_t)score;
}
