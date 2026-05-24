#include "app_ui.h"

#include "ssd1306.h"
#include "ssd1306_fonts.h"

#define APP_UI_PAGE_COUNT 5U

static void AppUI_DrawHome(void);
static void AppUI_DrawEnvironment(void);
static void AppUI_DrawWeather(void);
static void AppUI_DrawAlarm(void);
static void AppUI_DrawDevice(void);
static void AppUI_Clear(void);

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
    AppUI_Clear();
    ssd1306_WriteString("Home", Font_7x10, White);
    ssd1306_SetCursor(0, 16);
    ssd1306_WriteString("SmartDesk", Font_7x10, White);
    ssd1306_SetCursor(0, 32);
    ssd1306_WriteString("System Ready", Font_7x10, White);
    ssd1306_SetCursor(0, 48);
    ssd1306_WriteString("Mode: Auto", Font_7x10, White);
}

static void AppUI_DrawEnvironment(void)
{
    AppUI_Clear();
    ssd1306_WriteString("Environment", Font_7x10, White);
    ssd1306_SetCursor(0, 16);
    ssd1306_WriteString("Temp: 26C", Font_7x10, White);
    ssd1306_SetCursor(0, 32);
    ssd1306_WriteString("Humi: 55%", Font_7x10, White);
    ssd1306_SetCursor(0, 48);
    ssd1306_WriteString("Gas : Normal", Font_7x10, White);
}

static void AppUI_DrawWeather(void)
{
    AppUI_Clear();
    ssd1306_WriteString("Weather", Font_7x10, White);
    ssd1306_SetCursor(0, 16);
    ssd1306_WriteString("City: Xian", Font_7x10, White);
    ssd1306_SetCursor(0, 32);
    ssd1306_WriteString("Sunny 28C", Font_7x10, White);
    ssd1306_SetCursor(0, 48);
    ssd1306_WriteString("Wind: Low", Font_7x10, White);
}

static void AppUI_DrawAlarm(void)
{
    AppUI_Clear();
    ssd1306_WriteString("Alarm", Font_7x10, White);
    ssd1306_SetCursor(0, 16);
    ssd1306_WriteString("Time: 07:30", Font_7x10, White);
    ssd1306_SetCursor(0, 32);
    ssd1306_WriteString("Status: ON", Font_7x10, White);
    ssd1306_SetCursor(0, 48);
    ssd1306_WriteString("Repeat: Daily", Font_7x10, White);
}

static void AppUI_DrawDevice(void)
{
    AppUI_Clear();
    ssd1306_WriteString("Device", Font_7x10, White);
    ssd1306_SetCursor(0, 16);
    ssd1306_WriteString("Fan: OFF", Font_7x10, White);
    ssd1306_SetCursor(0, 32);
    ssd1306_WriteString("Light: OFF", Font_7x10, White);
    ssd1306_SetCursor(0, 48);
    ssd1306_WriteString("OLED: OK", Font_7x10, White);
}
