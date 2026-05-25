#include "app_esp8266.h"

#include "main.h"

#include <string.h>
#include <stdio.h>

#define APP_ESP8266_RX_BUFFER_SIZE 256U
#define APP_ESP8266_CMD_BUFFER_SIZE 96U
#define APP_ESP8266_SNTP_RETRY_COUNT 20U
#define APP_ESP8266_SNTP_QUERY_TIMEOUT_MS 3000U
#define APP_ESP8266_SNTP_RETRY_DELAY_MS 2000U
#define WIFI_SSID "OPPO"
#define WIFI_PASS "12345678"

extern UART_HandleTypeDef huart1;

static uint8_t esp_wifi_connected = 0U;

static uint8_t AppESP8266_SendATExpect2(const char *cmd, const char *expect1, const char *expect2, uint32_t timeout);
static uint8_t AppESP8266_SendCollect(const char *cmd, char *out, uint16_t out_len, uint32_t timeout);

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

uint8_t AppESP8266_PingTest(void)
{
    const char *cmds[] =
    {
        "AT+PING=\"ntp.aliyun.com\"\r\n",
        "AT+PING=\"ntp.tencent.com\"\r\n",
        "AT+PING=\"cn.ntp.org.cn\"\r\n"
    };
    char rx_buf[APP_ESP8266_RX_BUFFER_SIZE];
    uint8_t i;
    uint8_t ok = 0U;

    for (i = 0U; i < 3U; i++)
    {
        if (AppESP8266_SendCollect(cmds[i], rx_buf, sizeof(rx_buf), 5000U) == 0U)
        {
            printf("PING %u transmit failed\r\n", (unsigned int)(i + 1U));
        }

        printf("PING %u response:\r\n%s\r\n", (unsigned int)(i + 1U), rx_buf);

        if ((strstr(rx_buf, "+PING") != 0) || (strstr(rx_buf, "OK") != 0))
        {
            ok = 1U;
        }
    }

    return ok;
}

uint8_t AppESP8266_ConfigSNTP(void)
{
    char rx_buf[APP_ESP8266_RX_BUFFER_SIZE];
    uint8_t ok = 0U;

    AppESP8266_SendCollect("AT+CIPSNTPCFG=0\r\n", rx_buf, sizeof(rx_buf), 3000U);
    printf("SNTP disable response:\r\n%s\r\n", rx_buf);

    AppESP8266_SendCollect("AT+CIPSNTPCFG=1,8,\"ntp.aliyun.com\",\"ntp.tencent.com\",\"cn.ntp.org.cn\"\r\n",
                           rx_buf,
                           sizeof(rx_buf),
                           3000U);
    printf("SNTP config response:\r\n%s\r\n", rx_buf);

    if (strstr(rx_buf, "OK") != 0)
    {
        ok = 1U;
        HAL_Delay(5000U);
    }

    return ok;
}

uint8_t AppESP8266_GetSNTPTime(char *out, uint16_t out_len)
{
    uint8_t ch;
    uint8_t attempt;
    uint8_t found_time;
    uint16_t rx_len;
    uint32_t start_tick;

    if ((out == 0) || (out_len == 0U))
    {
        return 0U;
    }

    for (attempt = 0U; attempt < APP_ESP8266_SNTP_RETRY_COUNT; attempt++)
    {
        out[0] = '\0';
        rx_len = 0U;
        found_time = 0U;
        start_tick = HAL_GetTick();

        if (HAL_UART_Transmit(&huart1, (uint8_t *)"AT+CIPSNTPTIME?\r\n", (uint16_t)strlen("AT+CIPSNTPTIME?\r\n"), 100U) != HAL_OK)
        {
            printf("SNTP attempt %u transmit failed\r\n", (unsigned int)(attempt + 1U));
        }
        else
        {
            while ((HAL_GetTick() - start_tick) < APP_ESP8266_SNTP_QUERY_TIMEOUT_MS)
            {
                if (HAL_UART_Receive(&huart1, &ch, 1U, 10U) == HAL_OK)
                {
                    if (rx_len < (out_len - 1U))
                    {
                        out[rx_len] = (char)ch;
                        rx_len++;
                        out[rx_len] = '\0';
                    }

                    if (strstr(out, "+CIPSNTPTIME:") != 0)
                    {
                        found_time = 1U;
                    }

                    if ((found_time != 0U) && (strstr(out, "OK") != 0))
                    {
                        break;
                    }
                }
            }

            printf("SNTP attempt %u response:\r\n%s\r\n", (unsigned int)(attempt + 1U), out);

            if ((found_time != 0U) &&
                (strstr(out, "1970") == 0) &&
                (strstr(out, "Jan 1") == 0) &&
                (strstr(out, "Jan 01") == 0) &&
                (strstr(out, " 20") != 0))
            {
                return 1U;
            }
        }

        if ((attempt + 1U) < APP_ESP8266_SNTP_RETRY_COUNT)
        {
            HAL_Delay(APP_ESP8266_SNTP_RETRY_DELAY_MS);
        }
    }

    return 0U;
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

static uint8_t AppESP8266_SendCollect(const char *cmd, char *out, uint16_t out_len, uint32_t timeout)
{
    uint8_t ch;
    uint16_t rx_len = 0U;
    uint32_t start_tick;

    if ((out == 0) || (out_len == 0U))
    {
        return 0U;
    }

    out[0] = '\0';

    if (HAL_UART_Transmit(&huart1, (uint8_t *)cmd, (uint16_t)strlen(cmd), 100U) != HAL_OK)
    {
        return 0U;
    }

    start_tick = HAL_GetTick();
    while ((HAL_GetTick() - start_tick) < timeout)
    {
        if (HAL_UART_Receive(&huart1, &ch, 1U, 10U) == HAL_OK)
        {
            if (rx_len < (out_len - 1U))
            {
                out[rx_len] = (char)ch;
                rx_len++;
                out[rx_len] = '\0';
            }
        }
    }

    return 1U;
}
