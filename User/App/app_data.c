#include "app_data.h"

static SmartDesk_Data_t smartdesk_data;
static uint8_t fake_gas_index = 0U;

void AppData_Init(void)
{
    smartdesk_data.temperature = 26.0f;
    smartdesk_data.humidity = 55.0f;
    smartdesk_data.light_adc = 1200U;
    smartdesk_data.fan_on = 0U;
    smartdesk_data.light_on = 0U;
    smartdesk_data.alarm_on = 0U;
    smartdesk_data.mode = 0U;
    smartdesk_data.esp_ok = 0U;
    smartdesk_data.wifi_ok = 0U;
    fake_gas_index = 0U;
    AppData_UpdateFake();
}

SmartDesk_Data_t* AppData_Get(void)
{
    return &smartdesk_data;
}

void AppData_UpdateFake(void)
{
    static const uint16_t fake_gas_values[] = {800U, 1200U, 2000U};

    smartdesk_data.gas_adc = fake_gas_values[fake_gas_index];
    fake_gas_index++;
    if (fake_gas_index >= (sizeof(fake_gas_values) / sizeof(fake_gas_values[0])))
    {
        fake_gas_index = 0U;
    }

    if (smartdesk_data.gas_adc >= 1800U)
    {
        smartdesk_data.alarm_on = 1U;
    }
    else if (smartdesk_data.gas_adc < 1000U)
    {
        smartdesk_data.alarm_on = 0U;
    }

    smartdesk_data.comfort_score = AppData_CalcComfortScore(&smartdesk_data);
}

void AppData_NextMode(void)
{
    smartdesk_data.mode++;
    if (smartdesk_data.mode > 3U)
    {
        smartdesk_data.mode = 0U;
    }

    smartdesk_data.comfort_score = AppData_CalcComfortScore(&smartdesk_data);
}

const char* AppData_GetLightLevel(uint16_t light_adc)
{
    if (light_adc < 1800U)
    {
        return "Dark";
    }

    if (light_adc <= 2000U)
    {
        return "Normal";
    }

    return "Bright";
}

const char* AppData_GetGasLevel(uint16_t gas_adc)
{
    if (gas_adc < 1000U)
    {
        return "Normal";
    }

    if (gas_adc < 1800U)
    {
        return "Warning";
    }

    return "Danger";
}

const char* AppData_GetModeName(uint8_t mode)
{
    switch (mode)
    {
        case 0:
            return "Auto";

        case 1:
            return "Study";

        case 2:
            return "Sleep";

        case 3:
            return "Away";

        default:
            return "Unknown";
    }
}

uint8_t AppData_CalcComfortScore(SmartDesk_Data_t *data)
{
    int16_t score = 100;
    const char *light_level;
    const char *gas_level;
    uint8_t gas_warning_penalty = 20U;
    uint8_t gas_danger_penalty = 40U;
    uint8_t light_dark_penalty = 10U;
    uint8_t light_bright_penalty = 5U;

    if (data == 0)
    {
        return 0U;
    }

    switch (data->mode)
    {
        case 1:
            gas_warning_penalty = 30U;
            gas_danger_penalty = 55U;
            light_dark_penalty = 15U;
            light_bright_penalty = 10U;
            break;

        case 2:
            light_bright_penalty = 20U;
            break;

        case 3:
            gas_danger_penalty = 60U;
            break;

        case 0:
        default:
            break;
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

    gas_level = AppData_GetGasLevel(data->gas_adc);
    if (gas_level[0] == 'W')
    {
        score -= gas_warning_penalty;
    }
    else if (gas_level[0] == 'D')
    {
        score -= gas_danger_penalty;
    }

    light_level = AppData_GetLightLevel(data->light_adc);
    if (light_level[0] == 'D')
    {
        score -= light_dark_penalty;
    }
    else if (light_level[0] == 'B')
    {
        score -= light_bright_penalty;
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
