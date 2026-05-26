#include "app_ui.h"

#include "app_clock.h"
#include "app_data.h"
#include "app_weather.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"

#include <stdio.h>

static void AppUI_DrawHome(void);
static void AppUI_DrawEnvironment(void);
static void AppUI_DrawWeather(void);
static void AppUI_DrawAlarm(void);
static void AppUI_DrawDevice(void);
static void AppUI_Clear(void);
static void AppUI_FormatFloat1(char *buf, uint32_t size, const char *label, float value, const char *unit);
static void AppUI_FormatWeatherLine(char *buf, uint32_t size, const char *city, const char *weather);
static const char *AppUI_LightLevelShort(uint16_t light_adc);

void AppUI_Init(void)
{
    ssd1306_Init();
    ssd1306_Fill(Black);
    ssd1306_UpdateScreen();
}

void AppUI_ShowPage(uint8_t page)
{
    ssd1306_Fill(Black);
    ssd1306_SetCursor(0, 0);

    switch (page)
    {
        case 0:
            AppUI_DrawHome();
            break;

        case 1:
            AppUI_DrawEnvironment();
            break;

        case 2:
            AppUI_DrawDevice();
            break;

        case 3:
            AppUI_DrawAlarm();
            break;

        case 4:
            AppUI_DrawWeather();
            break;

        default:
            AppUI_DrawHome();
            break;
    }

    ssd1306_UpdateScreen();
}

static void AppUI_Clear(void)
{
    ssd1306_Fill(Black);
    ssd1306_SetCursor(0, 0);
}

static void AppUI_DrawHome(void)
{
    SmartDesk_Data_t *data = AppData_Get();
    char line[24];

    AppUI_Clear();
    ssd1306_WriteString("Home", Font_7x10, White);
    ssd1306_SetCursor(0, 16);
    ssd1306_WriteString("SmartDesk", Font_7x10, White);
    ssd1306_SetCursor(0, 32);
    ssd1306_WriteString("System Ready", Font_7x10, White);
    ssd1306_SetCursor(0, 48);
    snprintf(line, sizeof(line), "Mode:%s", AppData_GetModeName(data->mode));
    ssd1306_WriteString(line, Font_7x10, White);
}

static void AppUI_DrawEnvironment(void)
{
    SmartDesk_Data_t *data = AppData_Get();
    char line[24];

    AppUI_Clear();
    ssd1306_WriteString("Env", Font_7x10, White);

    ssd1306_SetCursor(0, 12);
    AppUI_FormatFloat1(line, sizeof(line), "T:", data->temperature, "");
    ssd1306_WriteString(line, Font_7x10, White);
    ssd1306_SetCursor(64, 12);
    snprintf(line, sizeof(line), "H:%u%%", (uint16_t)(data->humidity + 0.5f));
    ssd1306_WriteString(line, Font_7x10, White);

    ssd1306_SetCursor(0, 24);
    snprintf(line, sizeof(line), "G:%u %s", data->gas_adc, AppData_GetGasLevel(data->gas_adc));
    ssd1306_WriteString(line, Font_7x10, White);

    ssd1306_SetCursor(0, 36);
    snprintf(line, sizeof(line), "L:%u %s", data->light_adc, AppUI_LightLevelShort(data->light_adc));
    ssd1306_WriteString(line, Font_7x10, White);

    ssd1306_SetCursor(0, 48);
    snprintf(line, sizeof(line), "S:%u", data->comfort_score);
    ssd1306_WriteString(line, Font_7x10, White);
}

static void AppUI_DrawWeather(void)
{
    SmartDesk_Data_t *data = AppData_Get();
    AppWeather_Data_t *weather = AppWeather_Get();
    char line[24];

    AppUI_Clear();
    ssd1306_WriteString("Weather", Font_7x10, White);

    ssd1306_SetCursor(0, 16);
    if (weather->updating != 0U)
    {
        snprintf(line, sizeof(line), "Updating...");
    }
    else if (weather->valid != 0U)
    {
        AppUI_FormatWeatherLine(line, sizeof(line), weather->city, weather->weather);
    }
    else
    {
        snprintf(line, sizeof(line), "No Data");
    }
    ssd1306_WriteString(line, Font_7x10, White);

    if (weather->updating != 0U)
    {
        ssd1306_SetCursor(0, 48);
        snprintf(line, sizeof(line), "WiFi:%s", data->wifi_ok ? "OK" : "FAIL");
        ssd1306_WriteString(line, Font_7x10, White);
    }
    else if (weather->valid != 0U)
    {
        ssd1306_SetCursor(0, 32);
        snprintf(line, sizeof(line), "T:%dC", weather->temperature);
        ssd1306_WriteString(line, Font_7x10, White);

        ssd1306_SetCursor(0, 48);
        snprintf(line,
                 sizeof(line),
                 "Upd:%02u:%02u %s",
                 weather->last_update_hour,
                 weather->last_update_minute,
                 weather->update_ok ? "OK" : "Old");
        ssd1306_WriteString(line, Font_7x10, White);
    }
    else
    {
        ssd1306_SetCursor(0, 48);
        snprintf(line, sizeof(line), "WiFi:%s", data->wifi_ok ? "OK" : "FAIL");
        ssd1306_WriteString(line, Font_7x10, White);
    }
}

static void AppUI_DrawAlarm(void)
{
    AppClock_DateTime_t date_time = AppClock_GetDateTime();
    SmartDesk_Data_t *data = AppData_Get();
    char line[24];

    AppUI_Clear();
    ssd1306_WriteString("Alarm", Font_7x10, White);

    ssd1306_SetCursor(0, 12);
    snprintf(line, sizeof(line), "%02u-%02u-%02u %s",
             (uint8_t)(date_time.year % 100U),
             date_time.month,
             date_time.day,
             AppClock_GetWeekdayName(date_time.weekday));
    ssd1306_WriteString(line, Font_7x10, White);

    ssd1306_SetCursor(0, 24);
    snprintf(line, sizeof(line), "T:%02u:%02u:%02u", date_time.hour, date_time.minute, date_time.second);
    ssd1306_WriteString(line, Font_7x10, White);

    ssd1306_SetCursor(0, 36);
    snprintf(line, sizeof(line), "Sync:%s", data->time_synced ? "OK" : "FAIL");
    ssd1306_WriteString(line, Font_7x10, White);

    ssd1306_SetCursor(0, 48);
    snprintf(line, sizeof(line), "Stat:%s", AppClock_IsAlarmTriggered() ? "ON" : "OFF");
    ssd1306_WriteString(line, Font_7x10, White);
}

static void AppUI_DrawDevice(void)
{
    SmartDesk_Data_t *data = AppData_Get();
    char line[24];

    AppUI_Clear();
    ssd1306_WriteString("Device", Font_7x10, White);

    ssd1306_SetCursor(0, 12);
    snprintf(line, sizeof(line), "Fan:%s", data->fan_on ? "ON" : "OFF");
    ssd1306_WriteString(line, Font_7x10, White);

    ssd1306_SetCursor(0, 24);
    snprintf(line, sizeof(line), "Light:%s", data->light_on ? "ON" : "OFF");
    ssd1306_WriteString(line, Font_7x10, White);

    ssd1306_SetCursor(0, 36);
    snprintf(line, sizeof(line), "Alarm:%s", data->alarm_on ? "ON" : "OFF");
    ssd1306_WriteString(line, Font_7x10, White);

    ssd1306_SetCursor(0, 48);
    snprintf(line, sizeof(line), "Mode:%s", AppData_GetModeName(data->mode));
    ssd1306_WriteString(line, Font_7x10, White);
}

static void AppUI_FormatFloat1(char *buf, uint32_t size, const char *label, float value, const char *unit)
{
    int16_t scaled;
    int16_t integer;
    int16_t decimal;

    if (value >= 0.0f)
    {
        scaled = (int16_t)(value * 10.0f + 0.5f);
    }
    else
    {
        scaled = (int16_t)(value * 10.0f - 0.5f);
    }

    integer = scaled / 10;
    decimal = scaled % 10;
    if (decimal < 0)
    {
        decimal = -decimal;
    }

    snprintf(buf, size, "%s%d.%d%s", label, integer, decimal, unit);
}

static void AppUI_FormatWeatherLine(char *buf, uint32_t size, const char *city, const char *weather)
{
    char city_part[9];
    char weather_part[10];

    snprintf(city_part, sizeof(city_part), "%.8s", city);
    snprintf(weather_part, sizeof(weather_part), "%.9s", weather);
    snprintf(buf, size, "%s %s", city_part, weather_part);
}

static const char *AppUI_LightLevelShort(uint16_t light_adc)
{
    const char *level = AppData_GetLightLevel(light_adc);

    if (level[0] == 'N')
    {
        return "Nor";
    }

    if (level[0] == 'B')
    {
        return "Bri";
    }

    return "Dark";
}
