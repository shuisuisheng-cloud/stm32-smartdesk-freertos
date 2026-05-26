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
#define APP_ESP8266_HTTP_IDLE_TIMEOUT_MS 3000U
#define APP_ESP8266_RAW_RING_SIZE 2048U
#define WIFI_SSID "OPPO"
#define WIFI_PASS "12345678"

extern UART_HandleTypeDef huart1;

static uint8_t esp_wifi_connected = 0U;
static volatile uint8_t esp_raw_rx_active = 0U;
static volatile uint16_t esp_raw_rx_head = 0U;
static volatile uint16_t esp_raw_rx_tail = 0U;
static uint8_t esp_raw_rx_byte = 0U;
static uint8_t esp_raw_rx_ring[APP_ESP8266_RAW_RING_SIZE];

static void AppESP8266_ClearRx(void);
static uint8_t AppESP8266_SendATExpect2(const char *cmd, const char *expect1, const char *expect2, uint32_t timeout);
static uint8_t AppESP8266_SendCollect(const char *cmd, char *out, uint16_t out_len, uint32_t timeout);
static uint8_t AppESP8266_StartConnection(const char *type, const char *host, uint16_t port, char *out, uint16_t out_len);
static uint8_t AppESP8266_ConfirmTcpConnected(void);
static unsigned int AppESP8266_ParseIPDLength(const char *ipd);
static void AppESP8266_GetIPDStats(const char *buf, unsigned int *expected, unsigned int *received);
static void AppESP8266_RawRxBegin(void);
static void AppESP8266_RawRxEnd(void);
static uint8_t AppESP8266_RawRxPop(uint8_t *ch);

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
    char cipstart_first[65];
    uint16_t request_len;
    uint16_t cipstart_len;
    uint16_t copy_len;
    uint8_t http_seen = 0U;
    uint16_t port = use_ssl ? 443U : 80U;
    const char *type = use_ssl ? "SSL" : "TCP";
    uint8_t connected = 0U;

    if ((host == 0) || (path == 0) || (out == 0) || (out_len == 0U))
    {
        return 0U;
    }

    (void)timeout;
    out[0] = '\0';

    AppESP8266_SendATCollect("AT+CIPCLOSE\r\n", rx_buf, sizeof(rx_buf), 1500U);
    HAL_Delay(500U);
    AppESP8266_SendATCollect("AT+CIPMUX=0\r\n", rx_buf, sizeof(rx_buf), 2000U);
    AppESP8266_SendATCollect("AT+CIPMODE=0\r\n", rx_buf, sizeof(rx_buf), 2000U);
    printf("[WEATHER] pre cipstatus\r\n");
    AppESP8266_SendATCollect("AT+CIPSTATUS\r\n", rx_buf, sizeof(rx_buf), 2000U);

    printf("[WEATHER] cipstart begin\r\n");
    snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"%s\",\"%s\",%u\r\n", type, host, port);
    cipstart_len = AppESP8266_SendATCollect(cmd, rx_buf, sizeof(rx_buf), 8000U);
    copy_len = (cipstart_len > 64U) ? 64U : cipstart_len;
    memcpy(cipstart_first, rx_buf, copy_len);
    cipstart_first[copy_len] = '\0';
    printf("[WEATHER] cipstart raw len=%u\r\n", cipstart_len);
    printf("[WEATHER] cipstart raw first 64:\r\n%s\r\n", cipstart_first);

    if ((strstr(rx_buf, "CONNECT") != 0) ||
        (strstr(rx_buf, "OK") != 0) ||
        (strstr(rx_buf, "ALREADY CONNECTED") != 0))
    {
        connected = 1U;
    }

    if (connected == 0U)
    {
        AppESP8266_SendATCollect("AT+CIPSTATUS\r\n", rx_buf, sizeof(rx_buf), 3000U);
        if ((strstr(rx_buf, "STATUS:3") != 0) ||
            (strstr(rx_buf, "+CIPSTATUS:0,\"TCP\"") != 0))
        {
            connected = 1U;
            printf("[WEATHER] cipstatus confirm OK\r\n");
        }
        else
        {
            printf("[WEATHER] cipstatus confirm FAIL\r\n");
        }
    }

    if (connected == 0U)
    {
        printf("[WEATHER] cipstart fail\r\n");
        printf("[WEATHER] fail at cipstart\r\n");
        return 0U;
    }
    printf("[WEATHER] cipstart ok\r\n");

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
    printf("[WEATHER] cipsend begin len=%u\r\n", request_len);
    if (AppESP8266_SendCollect(cmd, rx_buf, sizeof(rx_buf), 3000U) == 0U)
    {
        printf("[WEATHER] cipsend prompt fail\r\n");
        printf("[WEATHER] fail at cipsend prompt\r\n");
        return 0U;
    }

    if (strstr(rx_buf, ">") == 0)
    {
        printf("[WEATHER] cipsend prompt fail\r\n");
        printf("[WEATHER] fail at cipsend prompt\r\n");
        return 0U;
    }
    printf("[WEATHER] cipsend prompt ok\r\n");

    AppESP8266_RawRxBegin();
    if (HAL_UART_Transmit(&huart1, (uint8_t *)request, request_len, 1000U) != HAL_OK)
    {
        AppESP8266_RawRxEnd();
        printf("[WEATHER] http tx fail\r\n");
        return 0U;
    }
    printf("[WEATHER] http tx ok\r\n");

    memset(out, 0, out_len);
    printf("[WEATHER] raw receive begin\r\n");
    (void)AppESP8266_ReadRawAppend(out, out_len, 20000U);

    http_seen = ((strstr(out, "+IPD,") != 0) ||
                 (strstr(out, "HTTP/1.1") != 0) ||
                 (strstr(out, "\"results\"") != 0) ||
                 ((strstr(out, "\"text\":\"") != 0) &&
                  (strstr(out, "\"temperature\":\"") != 0))) ? 1U : 0U;

    AppESP8266_SendCollect("AT+CIPCLOSE\r\n", rx_buf, sizeof(rx_buf), 1000U);

    if ((http_seen != 0U) &&
        ((strstr(out, "HTTP/1.1 200") != 0) ||
         (strstr(out, "HTTP/1.0 200") != 0) ||
         (strstr(out, "\"results\"") != 0) ||
         ((strstr(out, "\"text\":\"") != 0) &&
          (strstr(out, "\"temperature\":\"") != 0))))
    {
        return 1U;
    }

    return 0U;
}

uint16_t AppESP8266_SendATCollect(const char *cmd, char *buf, uint16_t size, uint32_t timeout_ms)
{
    uint8_t ch;
    uint16_t rx_len = 0U;
    uint32_t start_tick;

    if ((cmd == 0) || (buf == 0) || (size == 0U))
    {
        return 0U;
    }

    buf[0] = '\0';
    AppESP8266_ClearRx();

    if (HAL_UART_Transmit(&huart1, (uint8_t *)cmd, (uint16_t)strlen(cmd), 100U) != HAL_OK)
    {
        return 0U;
    }

    start_tick = HAL_GetTick();
    while ((HAL_GetTick() - start_tick) < timeout_ms)
    {
        if (HAL_UART_Receive(&huart1, &ch, 1U, 50U) == HAL_OK)
        {
            if (rx_len < (size - 1U))
            {
                buf[rx_len] = (char)ch;
                rx_len++;
                buf[rx_len] = '\0';
            }

            if (strstr(buf, "busy p") != 0)
            {
                HAL_Delay(500U);
            }
        }
    }

    return rx_len;
}

uint16_t AppESP8266_ReadRawResponse(char *buf, uint16_t buf_size, uint32_t timeout_ms)
{
    return AppESP8266_ReadRawAppend(buf, buf_size, timeout_ms);
}

uint16_t AppESP8266_ReadRawAppend(char *buf, uint16_t max_len, uint32_t total_timeout_ms)
{
    uint8_t ch;
    uint16_t rx_len = 0U;
    uint32_t start_tick;
    uint32_t last_rx_tick;
    uint32_t elapsed;
    uint8_t done = 0U;
    const char *end_reason = "timeout";
    unsigned int expected_ipd_len = 0U;
    unsigned int received_ipd_len = 0U;
    uint8_t has_http;
    uint8_t has_200;
    uint8_t has_results;
    uint8_t has_temp;

    if ((buf == 0) || (max_len == 0U))
    {
        return 0U;
    }

    rx_len = (uint16_t)strlen(buf);
    if (rx_len >= max_len)
    {
        rx_len = max_len - 1U;
        buf[rx_len] = '\0';
    }

    start_tick = HAL_GetTick();
    last_rx_tick = start_tick;

    while (((HAL_GetTick() - start_tick) < total_timeout_ms) && (done == 0U))
    {
        if (((esp_raw_rx_active != 0U) && (AppESP8266_RawRxPop(&ch) != 0U)) ||
            ((esp_raw_rx_active == 0U) && (HAL_UART_Receive(&huart1, &ch, 1U, 50U) == HAL_OK)))
        {
            last_rx_tick = HAL_GetTick();
            if (rx_len < (max_len - 1U))
            {
                buf[rx_len] = (char)ch;
                rx_len++;
                buf[rx_len] = '\0';
            }
            else
            {
                end_reason = "buffer_full";
                done = 1U;
            }

            AppESP8266_GetIPDStats(buf, &expected_ipd_len, &received_ipd_len);

            if ((done == 0U) && (strstr(buf, "CLOSED") != 0))
            {
                end_reason = "closed";
                done = 1U;
            }

            if ((done == 0U) &&
                (expected_ipd_len != 0U) &&
                (received_ipd_len >= expected_ipd_len))
            {
                end_reason = "ipd_complete";
                done = 1U;
            }

            if ((done == 0U) &&
                (expected_ipd_len == 0U) &&
                (strstr(buf, "\"results\"") != 0) &&
                (strstr(buf, "\"temperature\":\"") != 0))
            {
                end_reason = "ipd_complete";
                done = 1U;
            }
        }
        else
        {
            AppESP8266_GetIPDStats(buf, &expected_ipd_len, &received_ipd_len);
            if ((expected_ipd_len == 0U) &&
                (rx_len != 0U) &&
                ((HAL_GetTick() - last_rx_tick) >= APP_ESP8266_HTTP_IDLE_TIMEOUT_MS))
            {
                end_reason = "idle";
                done = 1U;
            }

            if (esp_raw_rx_active != 0U)
            {
                HAL_Delay(1U);
            }
        }
    }

    AppESP8266_RawRxEnd();
    elapsed = HAL_GetTick() - start_tick;
    has_http = (strstr(buf, "HTTP/1.1") != 0) ? 1U : 0U;
    has_200 = ((strstr(buf, "HTTP/1.1 200") != 0) || (strstr(buf, "HTTP/1.0 200") != 0)) ? 1U : 0U;
    has_results = (strstr(buf, "\"results\"") != 0) ? 1U : 0U;
    has_temp = (strstr(buf, "\"temperature\":\"") != 0) ? 1U : 0U;

    AppESP8266_GetIPDStats(buf, &expected_ipd_len, &received_ipd_len);

    printf("[WEATHER] raw rx len=%u\r\n", rx_len);
    if (rx_len == 0U)
    {
        printf("[WEATHER] no uart byte after http tx\r\n");
    }
    else
    {
        printf("[WEATHER] has HTTP=%d has 200=%d has results=%d has temp=%d\r\n",
               has_http,
               has_200,
               has_results,
               has_temp);
    }
    printf("[WEATHER] ipd expected=%u received=%u\r\n", expected_ipd_len, received_ipd_len);
    printf("[WEATHER] raw elapsed=%lu\r\n", elapsed);
    printf("[WEATHER] raw end reason=%s\r\n", end_reason);
    if ((strcmp(end_reason, "buffer_full") == 0) &&
        (expected_ipd_len != 0U) &&
        (received_ipd_len < expected_ipd_len))
    {
        printf("[WEATHER] buffer full before body complete\r\n");
    }

    return rx_len;
}

static unsigned int AppESP8266_ParseIPDLength(const char *ipd)
{
    unsigned int value = 0U;
    const char *p;

    if (ipd == 0)
    {
        return 0U;
    }

    p = strstr(ipd, "+IPD,");
    if (p == 0)
    {
        return 0U;
    }

    p += 5;
    while ((*p >= '0') && (*p <= '9'))
    {
        value = (value * 10U) + (unsigned int)(*p - '0');
        p++;
    }

    if (*p != ':')
    {
        return 0U;
    }

    return value;
}

static void AppESP8266_GetIPDStats(const char *buf, unsigned int *expected, unsigned int *received)
{
    const char *ipd;
    const char *payload;
    const char *next_ipd;
    unsigned int seg_expected;
    unsigned int seg_received;
    unsigned int total_expected = 0U;
    unsigned int total_received = 0U;

    if ((buf == 0) || (expected == 0) || (received == 0))
    {
        return;
    }

    ipd = strstr(buf, "+IPD,");
    while (ipd != 0)
    {
        seg_expected = AppESP8266_ParseIPDLength(ipd);
        payload = strchr(ipd, ':');
        if ((seg_expected == 0U) || (payload == 0))
        {
            break;
        }

        payload++;
        next_ipd = strstr(payload, "+IPD,");
        if (next_ipd != 0)
        {
            seg_received = (unsigned int)(next_ipd - payload);
        }
        else
        {
            seg_received = (unsigned int)strlen(payload);
        }

        total_expected += seg_expected;
        total_received += seg_received;
        ipd = next_ipd;
    }

    *expected = total_expected;
    *received = total_received;
}

static void AppESP8266_RawRxBegin(void)
{
    esp_raw_rx_head = 0U;
    esp_raw_rx_tail = 0U;
    esp_raw_rx_active = 1U;
    (void)HAL_UART_AbortReceive(&huart1);
    AppESP8266_ClearRx();
    HAL_NVIC_SetPriority(USART1_IRQn, 5U, 0U);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
    if (HAL_UART_Receive_IT(&huart1, &esp_raw_rx_byte, 1U) != HAL_OK)
    {
        esp_raw_rx_active = 0U;
    }
}

static void AppESP8266_RawRxEnd(void)
{
    esp_raw_rx_active = 0U;
    (void)HAL_UART_AbortReceive(&huart1);
}

static uint8_t AppESP8266_RawRxPop(uint8_t *ch)
{
    if ((ch == 0) || (esp_raw_rx_head == esp_raw_rx_tail))
    {
        return 0U;
    }

    *ch = esp_raw_rx_ring[esp_raw_rx_tail];
    esp_raw_rx_tail = (uint16_t)((esp_raw_rx_tail + 1U) % APP_ESP8266_RAW_RING_SIZE);
    return 1U;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    uint16_t next_head;

    if ((huart->Instance == USART1) && (esp_raw_rx_active != 0U))
    {
        next_head = (uint16_t)((esp_raw_rx_head + 1U) % APP_ESP8266_RAW_RING_SIZE);
        if (next_head != esp_raw_rx_tail)
        {
            esp_raw_rx_ring[esp_raw_rx_head] = esp_raw_rx_byte;
            esp_raw_rx_head = next_head;
        }

        (void)HAL_UART_Receive_IT(&huart1, &esp_raw_rx_byte, 1U);
    }
}

void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);
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
