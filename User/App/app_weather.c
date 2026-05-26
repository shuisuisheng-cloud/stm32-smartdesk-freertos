#include "app_weather.h"

#include "app_clock.h"
#include "app_data.h"
#include "app_esp8266.h"
#include "main.h"

#include <stdio.h>
#include <string.h>

#if defined(__has_include)
#if __has_include("app_weather_config.h")
#include "app_weather_config.h"
#else
#error "Copy User/App/app_weather_config.h.example to User/App/app_weather_config.h and set SENIVERSE_API_KEY."
#endif
#else
#include "app_weather_config.h"
#endif

#ifndef SENIVERSE_API_KEY
#error "Copy User/App/app_weather_config.h.example to User/App/app_weather_config.h and set SENIVERSE_API_KEY."
#endif

#ifndef SENIVERSE_LOCATION
#error "Copy User/App/app_weather_config.h.example to User/App/app_weather_config.h and set SENIVERSE_LOCATION."
#endif

#define APP_WEATHER_HOST "api.seniverse.com"
#define WEATHER_HTTP_RX_BUF_SIZE 2048U
#define APP_WEATHER_PATH_SIZE 192U
#define WEATHER_FIRST_UPDATE_DELAY_MS 5000UL
#define WEATHER_UPDATE_INTERVAL_MS 600000UL

static AppWeather_Data_t weather_data;
static char weather_http_rx_buf[WEATHER_HTTP_RX_BUF_SIZE];
static uint32_t weather_last_update_tick = 0U;
static uint8_t weather_update_pending = 1U;
static uint8_t weather_buf_size_printed = 0U;

static uint8_t AppWeather_Parse(const char *response);
static void Weather_DebugRawSummary(const char *buf, uint16_t len);
static uint8_t Weather_ParseStringField(const char *json, const char *key, char *out, uint16_t out_len);
static uint8_t Weather_ParseTemperature(const char *json, int *temperature);

void AppWeather_Init(void)
{
    memset(&weather_data, 0, sizeof(weather_data));
    weather_data.temperature = 0;
    weather_data.humidity = -1;
    weather_data.valid = 0U;
    weather_data.conn_fail = 0U;
    weather_data.last_update_hour = 0U;
    weather_data.last_update_minute = 0U;
    weather_data.update_ok = 0U;
    weather_data.updating = 0U;
    weather_last_update_tick = HAL_GetTick();
    weather_update_pending = 1U;
}

void AppWeather_Task(void)
{
    uint8_t ok;

    if (AppWeather_ShouldUpdate() != 0U)
    {
        weather_data.updating = 1U;
        ok = AppWeather_UpdateFromESP8266();
        weather_data.updating = 0U;
        printf("[NET] Weather %s\r\n", ok ? "OK" : "FAIL");
    }
}

uint8_t AppWeather_ShouldUpdate(void)
{
    SmartDesk_Data_t *data = AppData_Get();

    if ((data->wifi_ok == 0U) || (data->time_synced == 0U))
    {
        return 0U;
    }

    if (weather_update_pending != 0U)
    {
        if ((HAL_GetTick() - weather_last_update_tick) < WEATHER_FIRST_UPDATE_DELAY_MS)
        {
            return 0U;
        }

        return 1U;
    }

    if ((HAL_GetTick() - weather_last_update_tick) >= WEATHER_UPDATE_INTERVAL_MS)
    {
        return 1U;
    }

    return 0U;
}

uint8_t AppWeather_UpdateFromESP8266(void)
{
    char path[APP_WEATHER_PATH_SIZE];
    uint8_t ok = 0U;

    weather_data.updating = 1U;
    weather_last_update_tick = HAL_GetTick();
    weather_update_pending = 0U;
    weather_data.conn_fail = 0U;
    memset(weather_http_rx_buf, 0, sizeof(weather_http_rx_buf));
    snprintf(path,
             sizeof(path),
             "/v3/weather/now.json?key=%s&location=%s&language=en&unit=c",
             SENIVERSE_API_KEY,
             SENIVERSE_LOCATION);

    printf("[WEATHER] start\r\n");
    if (weather_buf_size_printed == 0U)
    {
        printf("[WEATHER] rx buf size=%u\r\n", (unsigned int)sizeof(weather_http_rx_buf));
        weather_buf_size_printed = 1U;
    }

    ok = AppESP8266_HTTPGet(APP_WEATHER_HOST, path, 0U, weather_http_rx_buf, sizeof(weather_http_rx_buf), 10000U);

    Weather_DebugRawSummary(weather_http_rx_buf, (uint16_t)strlen(weather_http_rx_buf));

    if (ok == 0U)
    {
        weather_data.conn_fail = 1U;
        weather_data.update_ok = 0U;
        goto cleanup;
    }

    ok = AppWeather_Parse(weather_http_rx_buf);
    if (ok == 0U)
    {
        weather_data.update_ok = 0U;
        goto cleanup;
    }

    ok = 1U;

cleanup:
    weather_data.updating = 0U;
    return ok;
}

AppWeather_Data_t* AppWeather_Get(void)
{
    return &weather_data;
}

static uint8_t AppWeather_Parse(const char *response)
{
    const char *json;
    int temperature;
    char city[sizeof(weather_data.city)];
    char weather[sizeof(weather_data.weather)];
    AppClock_DateTime_t date_time;

    if (response == 0)
    {
        return 0U;
    }

    json = strstr(response, "{\"results\"");
    if (json == 0)
    {
        json = strstr(response, "\"results\"");
    }

    if (json == 0)
    {
        json = strstr(response, "\"now\"");
    }

    if (json == 0)
    {
        json = strchr(response, '{');
    }

    if (json == 0)
    {
        return 0U;
    }

    if (Weather_ParseStringField(json, "\"text\":\"", weather, sizeof(weather)) == 0U)
    {
        return 0U;
    }

    if (Weather_ParseTemperature(json, &temperature) == 0U)
    {
        return 0U;
    }

    if (Weather_ParseStringField(json, "\"name\":\"", city, sizeof(city)) == 0U)
    {
        snprintf(city, sizeof(city), "Xian");
        printf("[WEATHER] city missing, fallback Xian\r\n");
    }

    memcpy(weather_data.city, city, sizeof(weather_data.city));
    memcpy(weather_data.weather, weather, sizeof(weather_data.weather));
    weather_data.temperature = (int16_t)temperature;
    weather_data.humidity = -1;
    weather_data.valid = 1U;
    weather_data.update_ok = 1U;
    weather_data.updating = 0U;
    weather_data.conn_fail = 0U;
    date_time = AppClock_GetDateTime();
    weather_data.last_update_hour = date_time.hour;
    weather_data.last_update_minute = date_time.minute;

    printf("[WEATHER] parsed city=%s text=%s temp=%d\r\n",
           weather_data.city,
           weather_data.weather,
           weather_data.temperature);

    return 1U;
}

static void Weather_DebugRawSummary(const char *buf, uint16_t len)
{
    char snippet[129];
    uint16_t copy_len;
    const char *ipd_start;
    const char *payload_start;
    unsigned int expected_ipd_len = 0U;
    unsigned int received_ipd_len = 0U;
    uint8_t has_results;
    uint8_t has_error;

    if (buf == 0)
    {
        return;
    }

    has_results = (strstr(buf, "\"results\"") != 0) ? 1U : 0U;
    has_error = ((strstr(buf, "status_code") != 0) ||
                 (strstr(buf, "error") != 0) ||
                 (strstr(buf, "invalid") != 0) ||
                 (strstr(buf, "forbidden") != 0) ||
                 (strstr(buf, "unauthorized") != 0)) ? 1U : 0U;

    if (has_error != 0U)
    {
        printf("[WEATHER] api error response\r\n");
    }

    ipd_start = strstr(buf, "+IPD,");
    if (ipd_start != 0)
    {
        ipd_start += 5;
        while ((*ipd_start >= '0') && (*ipd_start <= '9'))
        {
            expected_ipd_len = (expected_ipd_len * 10U) + (unsigned int)(*ipd_start - '0');
            ipd_start++;
        }
        payload_start = strchr(ipd_start, ':');
        if (payload_start != 0)
        {
            payload_start++;
            received_ipd_len = (unsigned int)strlen(payload_start);
        }

        if ((expected_ipd_len != 0U) && (received_ipd_len < expected_ipd_len))
        {
            printf("[WEATHER] body incomplete\r\n");
        }
    }

    if (has_results == 0U)
    {
        copy_len = (len > 128U) ? 128U : len;
        memcpy(snippet, buf, copy_len);
        snippet[copy_len] = '\0';
        printf("[WEATHER] raw first 128:\r\n%s\r\n", snippet);
    }
}

static uint8_t Weather_ParseStringField(const char *json, const char *key, char *out, uint16_t out_len)
{
    const char *start;
    const char *end;
    uint16_t len;

    if ((json == 0) || (key == 0) || (out == 0) || (out_len == 0U))
    {
        return 0U;
    }

    out[0] = '\0';
    start = strstr(json, key);
    if (start == 0)
    {
        return 0U;
    }

    start += strlen(key);
    end = strchr(start, '"');
    if (end == 0)
    {
        return 0U;
    }

    len = (uint16_t)(end - start);
    if (len >= out_len)
    {
        len = out_len - 1U;
    }

    memcpy(out, start, len);
    out[len] = '\0';
    return 1U;
}

static uint8_t Weather_ParseTemperature(const char *json, int *temperature)
{
    const char *start;

    if ((json == 0) || (temperature == 0))
    {
        return 0U;
    }

    start = strstr(json, "\"temperature\":\"");
    if (start == 0)
    {
        return 0U;
    }

    start += strlen("\"temperature\":\"");
    if (sscanf(start, "%d", temperature) != 1)
    {
        return 0U;
    }

    return 1U;
}
