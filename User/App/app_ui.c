#include "app_ui.h"

#include "app_clock.h"
#include "app_data.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"

#include <stdio.h>

#define APP_UI_PAGE_COUNT 5U

static void AppUI_DrawHome(void);
static void AppUI_DrawEnvironment(void);
static void AppUI_DrawWeather(void);
static void AppUI_DrawAlarm(void);
static void AppUI_DrawDevice(void);
static void AppUI_Clear(void);
static void AppUI_FormatFloat1(char *buf, uint32_t size, const char *label, float value, const char *unit);
static const char *AppUI_LightLevelShort(uint16_t light_adc);

void AppUI_Init(void)
{
    ssd1306_Init();
    ssd1306_Fill(Black);
    ssd1306_UpdateScreen();
}

void AppUI_ShowPage(uint8_t page)
{
    switch (page % APP_UI_PAGE_COUNT)
    {
        case 0:
            AppUI_DrawHome();
            break;

        case 1:
            AppUI_DrawEnvironment();
            break;

        case 2:
            AppUI_DrawWeather();
            break;

        case 3:
            AppUI_DrawAlarm();
            break;

        case 4:
        default:
            AppUI_DrawDevice();
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
    char line[24];

    AppUI_Clear();
    ssd1306_WriteString("Weather", Font_7x10, White);
    ssd1306_SetCursor(0, 16);
    snprintf(line, sizeof(line), "ESP:%s", data->esp_ok ? "OK" : "FAIL");
    ssd1306_WriteString(line, Font_7x10, White);
    ssd1306_SetCursor(0, 32);
    snprintf(line, sizeof(line), "WiFi:%s", data->wifi_ok ? "OK" : "FAIL");
    ssd1306_WriteString(line, Font_7x10, White);
}

static void AppUI_DrawAlarm(void)
{
    AppClock_Time_t time = AppClock_GetTime();
    SmartDesk_Data_t *data = AppData_Get();
    char line[24];

    AppUI_Clear();
    ssd1306_WriteString("Alarm", Font_7x10, White);

    ssd1306_SetCursor(0, 16);
    snprintf(line, sizeof(line), "Time:%02u:%02u:%02u", time.hour, time.minute, time.second);
    ssd1306_WriteString(line, Font_7x10, White);

    ssd1306_SetCursor(0, 32);
    snprintf(line, sizeof(line), "Sync:%s", data->time_synced ? "OK" : "FAIL");
    ssd1306_WriteString(line, Font_7x10, White);

    ssd1306_SetCursor(0, 48);
    snprintf(line, sizeof(line), "Status:%s", AppClock_IsAlarmTriggered() ? "ON" : "OFF");
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
