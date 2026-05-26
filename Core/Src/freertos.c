/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "cmsis_os.h"
#include "app_ui.h"
#include "app_key.h"
#include "app_data.h"
#include "app_clock.h"
#include "app_sensor.h"
#include "app_actuator.h"
#include "app_esp8266.h"
#include "app_weather.h"
#include "app_voice.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
static uint8_t current_page = 0U;
static uint32_t app_core_slow_tick = 0U;
static uint32_t app_core_ui_tick = 0U;
static uint32_t app_core_voice_tick = 0U;
static GPIO_PinState last_pc13_raw = GPIO_PIN_SET;
static uint8_t oled_i2c_ready = 0U;
extern I2C_HandleTypeDef hi2c1;
extern void App_I2C1_ReInit(void);
extern uint8_t App_I2C1_Scan(void);
extern void App_I2C1_BusRecover(void);

/* USER CODE END Variables */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void AppFreeRTOS_AppCoreTaskLoop(void);
void AppFreeRTOS_WeatherTaskLoop(void);
static void Debug_OLED_CheckAndInit(void);
static uint8_t AppFreeRTOS_TrySNTP(char *buf, uint16_t buf_len);
static uint8_t AppFreeRTOS_TryWeather(void);

/* USER CODE END FunctionPrototypes */

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
static void Debug_OLED_CheckAndInit(void)
{
    uint8_t found_addr;

    osDelay(500);

    found_addr = App_I2C1_Scan();

    if ((found_addr != 0x3CU) && (found_addr != 0x3DU))
    {
        App_I2C1_BusRecover();
        found_addr = App_I2C1_Scan();
    }

    if ((found_addr == 0x3CU) || (found_addr == 0x3DU))
    {
        oled_i2c_ready = 1U;
        ssd1306_Init();
        ssd1306_Fill(Black);
        ssd1306_SetCursor(0, 0);
        ssd1306_WriteString("OLED RECOVER", Font_7x10, White);
        ssd1306_UpdateScreen();
        printf("[OLED] OK\r\n");
    }
    else
    {
        oled_i2c_ready = 0U;
        printf("[OLED] FAIL\r\n");
    }
}

static uint8_t AppFreeRTOS_TrySNTP(char *buf, uint16_t buf_len)
{
    uint8_t sntp_cfg_ok;
    uint8_t sntp_time_ok;
    uint8_t clock_sync_ok;

    sntp_cfg_ok = AppESP8266_ConfigSNTP();

    if (sntp_cfg_ok == 0U)
    {
        printf("[NET] SNTP FAIL\r\n");
        AppData_Get()->time_synced = 0U;
        return 0U;
    }

    sntp_time_ok = AppESP8266_GetSNTPTime(buf, buf_len);

    if (sntp_time_ok == 0U)
    {
        printf("[NET] SNTP FAIL\r\n");
        AppData_Get()->time_synced = 0U;
        return 0U;
    }

    clock_sync_ok = AppClock_SetTimeFromSNTPString(buf);
    printf("[NET] SNTP %s\r\n", clock_sync_ok ? "OK" : "FAIL");
    AppData_Get()->time_synced = clock_sync_ok ? 1U : 0U;

    return clock_sync_ok;
}

static uint8_t AppFreeRTOS_TryWeather(void)
{
    uint8_t ok;

    AppWeather_Get()->updating = 1U;
    ok = AppWeather_UpdateFromESP8266();
    AppWeather_Get()->updating = 0U;

    printf("[NET] Weather %s\r\n", ok ? "OK" : "FAIL");
    return ok;
}

void AppFreeRTOS_AppCoreTaskLoop(void)
{
    uint8_t key_event;
    GPIO_PinState pc13_raw;

    app_core_slow_tick = HAL_GetTick();
    app_core_ui_tick = HAL_GetTick();
    app_core_voice_tick = HAL_GetTick();
    AppKey_Init();
    AppVoice_Init();
    last_pc13_raw = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);
    Debug_OLED_CheckAndInit();
    if (oled_i2c_ready != 0U)
    {
        AppUI_ShowPage(current_page);
    }

    for (;;)
    {
        AppActuator_Update();

        pc13_raw = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);
        if (pc13_raw != last_pc13_raw)
        {
            last_pc13_raw = pc13_raw;
        }

        if ((HAL_GetTick() - app_core_slow_tick) >= 100U)
        {
            app_core_slow_tick = HAL_GetTick();
            AppClock_Update();
            AppSensor_UpdateData();
        }

        if ((HAL_GetTick() - app_core_voice_tick) >= 50U)
        {
            uint8_t voice_cmd;

            app_core_voice_tick = HAL_GetTick();
            AppVoice_Update();
            voice_cmd = AppVoice_GetLastCommand();

            if (voice_cmd != APP_VOICE_CMD_NONE)
            {
                if (voice_cmd == APP_VOICE_CMD_PAGE_NEXT)
                {
                    current_page = (uint8_t)((current_page + 1U) % APP_UI_PAGE_COUNT);
                    printf("[UI] page=%d\r\n", current_page);
                }
                else if (voice_cmd == APP_VOICE_CMD_MODE_AUTO)
                {
                    AppData_SetMode(0U);
                    printf("[MODE] changed\r\n");
                }
                else if (voice_cmd == APP_VOICE_CMD_MODE_MANUAL)
                {
                    AppData_SetMode(1U);
                    printf("[MODE] changed\r\n");
                }
                else if (voice_cmd == APP_VOICE_CMD_ALARM_OFF)
                {
                    AppClock_ClearAlarm();
                    AppActuator_BuzzerOff();
                }
                else if (voice_cmd == APP_VOICE_CMD_WEATHER_PAGE)
                {
                    current_page = 4U;
                    printf("[UI] page=%d\r\n", current_page);
                }

                AppVoice_ClearCommand();
                if (oled_i2c_ready != 0U)
                {
                    AppUI_ShowPage(current_page);
                }
            }
        }

        key_event = AppKey_Scan();

        if (key_event == APP_KEY_EVENT_SHORT)
        {
            current_page = (uint8_t)((current_page + 1U) % APP_UI_PAGE_COUNT);
            printf("[UI] page=%d\r\n", current_page);
            if (oled_i2c_ready != 0U)
            {
                AppUI_ShowPage(current_page);
            }
        }
        else if (key_event == APP_KEY_EVENT_LONG)
        {
            AppData_NextMode();
            printf("[MODE] changed\r\n");
            if (oled_i2c_ready != 0U)
            {
                AppUI_ShowPage(current_page);
            }
        }

        if ((HAL_GetTick() - app_core_ui_tick) >= 200U)
        {
            app_core_ui_tick = HAL_GetTick();
            if (oled_i2c_ready != 0U)
            {
                AppUI_ShowPage(current_page);
            }
        }

        osDelay(10);
    }
}

void AppFreeRTOS_WeatherTaskLoop(void)
{
    uint8_t at_ok;
    uint8_t connect_ok;
    uint8_t status_ok;
    uint8_t wifi_ok = 0U;
    uint8_t sntp_synced = 0U;
    uint8_t weather_started = 0U;
    char sntp_time_buf[128];
    uint32_t last_wifi_retry_tick = 0U;
    uint32_t last_wifi_status_tick = 0U;
    uint32_t last_weather_loop_log_tick = 0U;

    osDelay(5000);

    for (;;)
    {
        if (wifi_ok == 0U)
        {
            if ((last_wifi_retry_tick == 0U) || ((HAL_GetTick() - last_wifi_retry_tick) >= 10000U))
            {
                last_wifi_retry_tick = HAL_GetTick();

                at_ok = AppESP8266_TestAT();
                printf("[NET] ESP AT %s\r\n", at_ok ? "OK" : "FAIL");

                if (at_ok != 0U)
                {
                    AppESP8266_SetStationMode();
                    connect_ok = AppESP8266_ConnectWiFi(0, 0);

                    status_ok = AppESP8266_CheckWiFiStatus();

                    wifi_ok = ((connect_ok != 0U) || (status_ok != 0U)) ? 1U : 0U;
                    printf("[NET] WiFi %s\r\n", wifi_ok ? "OK" : "FAIL");
                }
                else
                {
                    connect_ok = 0U;
                    status_ok = AppESP8266_CheckWiFiStatus();
                    wifi_ok = status_ok;
                    printf("[NET] WiFi %s\r\n", wifi_ok ? "OK" : "FAIL");
                }

                AppData_Get()->wifi_ok = wifi_ok;
                last_wifi_status_tick = HAL_GetTick();

                if ((wifi_ok != 0U) && (sntp_synced == 0U))
                {
                    sntp_synced = AppFreeRTOS_TrySNTP(sntp_time_buf, sizeof(sntp_time_buf));
                    if (sntp_synced != 0U)
                    {
                        weather_started = AppFreeRTOS_TryWeather();
                    }
                }
            }
        }
        else
        {
            if ((last_weather_loop_log_tick == 0U) ||
                ((HAL_GetTick() - last_weather_loop_log_tick) >= 30000U))
            {
                last_weather_loop_log_tick = HAL_GetTick();
                printf("[RTOS] weatherTask loop wifi_ok=%d\r\n", wifi_ok);
            }

            if ((HAL_GetTick() - last_wifi_status_tick) >= 30000U)
            {
                last_wifi_status_tick = HAL_GetTick();
                status_ok = AppESP8266_CheckWiFiStatus();

                wifi_ok = status_ok;
                printf("[NET] WiFi %s\r\n", wifi_ok ? "OK" : "FAIL");
                AppData_Get()->wifi_ok = wifi_ok;
                if (wifi_ok == 0U)
                {
                    sntp_synced = 0U;
                    weather_started = 0U;
                    AppData_Get()->time_synced = 0U;
                    last_wifi_retry_tick = HAL_GetTick();
                }
                else if (sntp_synced == 0U)
                {
                    sntp_synced = AppFreeRTOS_TrySNTP(sntp_time_buf, sizeof(sntp_time_buf));
                    if (sntp_synced != 0U)
                    {
                        weather_started = AppFreeRTOS_TryWeather();
                    }
                }
                else if (weather_started == 0U)
                {
                    weather_started = AppFreeRTOS_TryWeather();
                }
            }

            if (wifi_ok != 0U)
            {
                AppWeather_Task();
            }
        }

        osDelay(1000);
    }
}

/* USER CODE END Application */

