#include "app_esp8266.h"

#include "main.h"

#include <string.h>
#include <stdio.h>

#define APP_ESP8266_RX_BUFFER_SIZE 256U
#define APP_ESP8266_CMD_BUFFER_SIZE 96U
#define APP_ESP8266_HTTP_REQ_SIZE 384U
#define APP_ESP8266_SNTP_RETRY_COUNT 10U
#define APP_ESP8266_SNTP_QUERY_TIMEOUT_MS 3000U
#define APP_ESP8266_SNTP_RETRY_DELAY_MS 2000U
#define WIFI_SSID "OPPO"
#define WIFI_PASS "12345678"

extern UART_HandleTypeDef huart1;

static uint8_t esp_wifi_connected = 0U;

static void AppESP8266_ClearRx(void);
static uint8_t AppESP8266_SendATExpect2(const char *cmd, const char *expect1, const char *expect2, uint32_t timeout);
static uint8_t AppESP8266_SendCollect(const char *cmd, char *out, uint16_t out_len, uint32_t timeout);
static uint8_t AppESP8266_StartConnection(const char *type, const char *host, uint16_t port, char *out, uint16_t out_len);
static uint8_t AppESP8266_ConfirmTcpConnected(void);

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
    esp_wifi_connected = AppESP8266_SendATExpect2(cmd, "WIFI CONNECTED", "OK", 30000U);

    return esp_wifi_connected;
}

uint8_t AppESP8266_IsWiFiConnected(void)
{
    return esp_wifi_connected;
}

uint8_t AppESP8266_CheckWiFiStatus(void)
{
    char rx_buf[APP_ESP8266_RX_BUFFER_SIZE];
    uint8_t ok = 0U;

    AppESP8266_SendCollect("AT+CWJAP?\r\n", rx_buf, sizeof(rx_buf), 3000U);
    if (strstr(rx_buf, WIFI_SSID) != 0)
    {
        ok = 1U;
    }

    AppESP8266_SendCollect("AT+CIPSTATUS\r\n", rx_buf, sizeof(rx_buf), 3000U);
    if ((strstr(rx_buf, "STATUS:2") != 0) || (strstr(rx_buf, "STATUS:3") != 0))
    {
        ok = 1U;
    }

    AppESP8266_SendCollect("AT+CIPSTA?\r\n", rx_buf, sizeof(rx_buf), 3000U);
    if (strstr(rx_buf, "ip:") != 0)
    {
        ok = 1U;
    }

    esp_wifi_connected = ok;
    return ok;
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
        }

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

    AppESP8266_SendCollect("AT+CIPSNTPCFG=1,8,\"ntp.aliyun.com\",\"ntp.tencent.com\",\"cn.ntp.org.cn\"\r\n",
                           rx_buf,
                           sizeof(rx_buf),
                           3000U);

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
        AppESP8266_ClearRx();

        if (HAL_UART_Transmit(&huart1, (uint8_t *)"AT+CIPSNTPTIME?\r\n", (uint16_t)strlen("AT+CIPSNTPTIME?\r\n"), 100U) != HAL_OK)
        {
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

uint8_t AppESP8266_PingHost(const char *host)
{
    char cmd[APP_ESP8266_CMD_BUFFER_SIZE];
    char rx_buf[APP_ESP8266_RX_BUFFER_SIZE];

    if (host == 0)
    {
        return 0U;
    }

    snprintf(cmd, sizeof(cmd), "AT+PING=\"%s\"\r\n", host);
    if (AppESP8266_SendCollect(cmd, rx_buf, sizeof(rx_buf), 6000U) == 0U)
    {
        return 0U;
    }

    if ((strstr(rx_buf, "+PING") != 0) || (strstr(rx_buf, "OK") != 0))
    {
        return 1U;
    }

    return 0U;
}

uint8_t AppESP8266_TestTcpConnect(const char *host, uint16_t port)
{
    char rx_buf[APP_ESP8266_RX_BUFFER_SIZE];
    uint8_t ok;

    if (host == 0)
    {
        return 0U;
    }

    ok = AppESP8266_StartConnection("TCP", host, port, rx_buf, sizeof(rx_buf));
    if (ok != 0U)
    {
        AppESP8266_SendCollect("AT+CIPCLOSE\r\n", rx_buf, sizeof(rx_buf), 1500U);
    }

    return ok;
}

uint8_t AppESP8266_HTTPGet(const char *host, const char *path, uint8_t use_ssl, char *out, uint16_t out_len, uint32_t timeout)
{
    char cmd[APP_ESP8266_CMD_BUFFER_SIZE];
    char request[APP_ESP8266_HTTP_REQ_SIZE];
    char rx_buf[APP_ESP8266_RX_BUFFER_SIZE];
    uint8_t ch;
    uint16_t request_len;
    uint16_t rx_len;
    uint32_t start_tick;
    uint8_t http_seen = 0U;
    uint16_t port = use_ssl ? 443U : 80U;
    const char *type = use_ssl ? "SSL" : "TCP";

    if ((host == 0) || (path == 0) || (out == 0) || (out_len == 0U))
    {
        return 0U;
    }

    (void)timeout;
    out[0] = '\0';

    AppESP8266_SendCollect("AT+CIPCLOSE\r\n", rx_buf, sizeof(rx_buf), 1500U);
    AppESP8266_SendCollect("AT+CIPMUX=0\r\n", rx_buf, sizeof(rx_buf), 2000U);
    AppESP8266_SendCollect("AT+CIPMODE=0\r\n", rx_buf, sizeof(rx_buf), 2000U);

    if (AppESP8266_StartConnection(type, host, port, rx_buf, sizeof(rx_buf)) == 0U)
    {
        return 0U;
    }

    request_len = (uint16_t)snprintf(request,
                                     sizeof(request),
                                     "GET %s HTTP/1.1\r\n"
                                     "Host: %s\r\n"
                                     "User-Agent: STM32-ESP8266\r\n"
                                     "Connection: close\r\n"
                                     "\r\n",
                                     path,
                                     host);
    if ((request_len == 0U) || (request_len >= sizeof(request)))
    {
        return 0U;
    }

    request_len = (uint16_t)strlen(request);
    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%u\r\n", request_len);
    if (AppESP8266_SendCollect(cmd, rx_buf, sizeof(rx_buf), 3000U) == 0U)
    {
        return 0U;
    }

    if (strstr(rx_buf, ">") == 0)
    {
        return 0U;
    }

    if (HAL_UART_Transmit(&huart1, (uint8_t *)request, request_len, 1000U) != HAL_OK)
    {
        return 0U;
    }

    memset(rx_buf, 0, sizeof(rx_buf));
    rx_len = 0U;
    start_tick = HAL_GetTick();
    while ((HAL_GetTick() - start_tick) < 5000U)
    {
        if (HAL_UART_Receive(&huart1, &ch, 1U, 20U) == HAL_OK)
        {
            if (rx_len < (sizeof(rx_buf) - 1U))
            {
                rx_buf[rx_len] = (char)ch;
                rx_len++;
                rx_buf[rx_len] = '\0';
            }

            if (strstr(rx_buf, "SEND OK") != 0)
            {
                break;
            }
        }
    }

    if (strstr(rx_buf, "SEND OK") == 0)
    {
        AppESP8266_SendCollect("AT+CIPCLOSE\r\n", rx_buf, sizeof(rx_buf), 1000U);
        return 0U;
    }

    memset(out, 0, out_len);
    (void)AppESP8266_ReadRawResponse(out, out_len, 20000U);

    http_seen = ((strstr(out, "+IPD,") != 0) ||
                 (strstr(out, "HTTP/1.1") != 0) ||
                 (strstr(out, "{\"results\"") != 0)) ? 1U : 0U;

    AppESP8266_SendCollect("AT+CIPCLOSE\r\n", rx_buf, sizeof(rx_buf), 1000U);

    if ((http_seen != 0U) &&
        ((strstr(out, "HTTP/1.1 200") != 0) ||
         (strstr(out, "HTTP/1.0 200") != 0) ||
         (strstr(out, "{\"results\"") != 0)))
    {
        return 1U;
    }

    return 0U;
}

uint16_t AppESP8266_ReadRawResponse(char *buf, uint16_t buf_size, uint32_t timeout_ms)
{
    uint8_t ch;
    uint16_t rx_len = 0U;
    uint32_t start_tick;
    uint8_t done = 0U;
    uint8_t has_results;

    if ((buf == 0) || (buf_size == 0U))
    {
        return 0U;
    }

    buf[0] = '\0';
    start_tick = HAL_GetTick();

    while (((HAL_GetTick() - start_tick) < timeout_ms) && (done == 0U))
    {
        if (HAL_UART_Receive(&huart1, &ch, 1U, 50U) == HAL_OK)
        {
            if (rx_len < (buf_size - 1U))
            {
                buf[rx_len] = (char)ch;
                rx_len++;
                buf[rx_len] = '\0';
            }

            if ((strstr(buf, "CLOSED") != 0) ||
                ((strstr(buf, "{\"results\"") != 0) && (strstr(buf, "\"temperature\":\"") != 0)))
            {
                done = 1U;
            }
        }
    }

    has_results = (strstr(buf, "{\"results\"") != 0) ? 1U : 0U;

    printf("[WEATHER] raw rx len=%u\r\n", rx_len);
    printf("[WEATHER] has results=%d\r\n", has_results);

    return rx_len;
}

static uint8_t AppESP8266_StartConnection(const char *type, const char *host, uint16_t port, char *out, uint16_t out_len)
{
    char cmd[APP_ESP8266_CMD_BUFFER_SIZE];
    uint8_t connected;

    if ((type == 0) || (host == 0) || (out == 0) || (out_len == 0U))
    {
        return 0U;
    }

    AppESP8266_SendCollect("AT+CIPCLOSE\r\n", out, out_len, 1500U);
    HAL_Delay(500U);

    snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"%s\",\"%s\",%u\r\n", type, host, port);
    if (AppESP8266_SendCollect(cmd, out, out_len, 10000U) == 0U)
    {
        return 0U;
    }

    if ((strstr(out, "ERROR") != 0) || (strstr(out, "FAIL") != 0))
    {
        connected = AppESP8266_ConfirmTcpConnected();
        return connected;
    }

    if ((strstr(out, "OK") != 0) ||
        (strstr(out, "CONNECT") != 0) ||
        (strstr(out, "ALREADY CONNECTED") != 0) ||
        (strstr(out, "STATUS:3") != 0))
    {
        return 1U;
    }

    connected = AppESP8266_ConfirmTcpConnected();
    return connected;
}

static uint8_t AppESP8266_ConfirmTcpConnected(void)
{
    char status_buf[APP_ESP8266_RX_BUFFER_SIZE];

    if (AppESP8266_SendCollect("AT+CIPSTATUS\r\n", status_buf, sizeof(status_buf), 3000U) == 0U)
    {
        return 0U;
    }

    if ((strstr(status_buf, "STATUS:3") != 0) ||
        (strstr(status_buf, "+CIPSTATUS:0,\"TCP\"") != 0))
    {
        return 1U;
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
    AppESP8266_ClearRx();

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

            if (strstr(rx_buf, "busy p") != 0)
            {
                HAL_Delay(500U);
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
    uint32_t last_rx_tick;

    if ((out == 0) || (out_len == 0U))
    {
        return 0U;
    }

    out[0] = '\0';
    AppESP8266_ClearRx();

    if (HAL_UART_Transmit(&huart1, (uint8_t *)cmd, (uint16_t)strlen(cmd), 100U) != HAL_OK)
    {
        return 0U;
    }

    start_tick = HAL_GetTick();
    last_rx_tick = start_tick;
    while ((HAL_GetTick() - start_tick) < timeout)
    {
        if (HAL_UART_Receive(&huart1, &ch, 1U, 10U) == HAL_OK)
        {
            last_rx_tick = HAL_GetTick();
            if (rx_len < (out_len - 1U))
            {
                out[rx_len] = (char)ch;
                rx_len++;
                out[rx_len] = '\0';
            }

            if (strstr(out, "busy p") != 0)
            {
                HAL_Delay(500U);
            }
        }
        else if ((rx_len != 0U) && ((HAL_GetTick() - last_rx_tick) >= 300U))
        {
            if ((strstr(out, "OK") != 0) ||
                (strstr(out, "ERROR") != 0) ||
                (strstr(out, "FAIL") != 0) ||
                (strstr(out, "CONNECT") != 0) ||
                (strstr(out, "ALREADY CONNECTED") != 0) ||
                (strstr(out, ">") != 0))
            {
                break;
            }
        }
    }

    return 1U;
}

static void AppESP8266_ClearRx(void)
{
    uint8_t ch;

    while (HAL_UART_Receive(&huart1, &ch, 1U, 0U) == HAL_OK)
    {
    }
}
