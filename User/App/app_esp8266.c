#include "app_esp8266.h"

#include "main.h"

#include <string.h>
#include <stdio.h>

#define APP_ESP8266_RX_BUFFER_SIZE 128U
#define APP_ESP8266_CMD_BUFFER_SIZE 96U
#define WIFI_SSID "OPPO"
#define WIFI_PASS "12345678"

extern UART_HandleTypeDef huart1;

static uint8_t esp_wifi_connected = 0U;

static uint8_t AppESP8266_SendATExpect2(const char *cmd, const char *expect1, const char *expect2, uint32_t timeout);

void AppESP8266_Init(void)
{
    esp_wifi_connected = 0U;
}

uint8_t AppESP8266_SendAT(const char *cmd, const char *expect, uint32_t timeout)
{
    return AppESP8266_SendATExpect2(cmd, expect, 0, timeout);
}

uint8_t AppESP8266_TestAT(void)
{
    return AppESP8266_SendAT("AT\r\n", "OK", 1000U);
}

uint8_t AppESP8266_SetStationMode(void)
{
    return AppESP8266_SendAT("AT+CWMODE=1\r\n", "OK", 2000U);
}

uint8_t AppESP8266_ConnectWiFi(const char *ssid, const char *password)
{
    char cmd[APP_ESP8266_CMD_BUFFER_SIZE];

    if (ssid == 0)
    {
        ssid = WIFI_SSID;
    }

    if (password == 0)
    {
        password = WIFI_PASS;
    }

    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);
    esp_wifi_connected = AppESP8266_SendATExpect2(cmd, "WIFI CONNECTED", "OK", 15000U);

    return esp_wifi_connected;
}

uint8_t AppESP8266_IsWiFiConnected(void)
{
    return esp_wifi_connected;
}

static uint8_t AppESP8266_SendATExpect2(const char *cmd, const char *expect1, const char *expect2, uint32_t timeout)
{
    uint8_t ch;
    char rx_buf[APP_ESP8266_RX_BUFFER_SIZE];
    uint16_t rx_len = 0U;
    uint32_t start_tick = HAL_GetTick();

    memset(rx_buf, 0, sizeof(rx_buf));

    if (HAL_UART_Transmit(&huart1, (uint8_t *)cmd, (uint16_t)strlen(cmd), 100U) != HAL_OK)
    {
        return 0U;
    }

    while ((HAL_GetTick() - start_tick) < timeout)
    {
        if (HAL_UART_Receive(&huart1, &ch, 1U, 10U) == HAL_OK)
        {
            if (rx_len < (APP_ESP8266_RX_BUFFER_SIZE - 1U))
            {
                rx_buf[rx_len] = (char)ch;
                rx_len++;
                rx_buf[rx_len] = '\0';
            }

            if ((expect1 != 0) && (strstr(rx_buf, expect1) != 0))
            {
                return 1U;
            }

            if ((expect2 != 0) && (strstr(rx_buf, expect2) != 0))
            {
                return 1U;
            }
        }
    }

    return 0U;
}
