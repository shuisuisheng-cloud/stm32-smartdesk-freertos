#include "app_weather.h"

#include "app_esp8266.h"

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
#define APP_WEATHER_HTTP_BUFFER_SIZE 768U
#define APP_WEATHER_PATH_SIZE 192U

static AppWeather_Data_t weather_data;
static char weather_http_buf[APP_WEATHER_HTTP_BUFFER_SIZE];

static uint8_t AppWeather_Parse(const char *response);
static uint8_t Weather_ParseStringField(const char *json, const char *key, char *out, uint16_t out_len);
static uint8_t Weather_ParseTemperature(const char *json, int *temperature);

void AppWeather_Init(void)
{
    memset(&weather_data, 0, sizeof(weather_data));
    weather_data.temperature = 0;
    weather_data.humidity = -1;
    weather_data.valid = 0U;
    weather_data.conn_fail = 0U;
}

uint8_t AppWeather_UpdateFromESP8266(void)
{
    char path[APP_WEATHER_PATH_SIZE];
    uint8_t ok;

    weather_data.valid = 0U;
    weather_data.conn_fail = 0U;
    snprintf(path,
             sizeof(path),
             "/v3/weather/now.json?key=%s&location=%s&language=en&unit=c",
             SENIVERSE_API_KEY,
             SENIVERSE_LOCATION);

    AppESP8266_PingHost(APP_WEATHER_HOST);
    AppESP8266_TestTcpConnect("www.baidu.com", 80U);
    if (AppESP8266_TestTcpConnect(APP_WEATHER_HOST, 80U) == 0U)
    {
        weather_data.conn_fail = 1U;
        return 0U;
    }

    ok = AppESP8266_HTTPGet(APP_WEATHER_HOST, path, 1U, weather_http_buf, sizeof(weather_http_buf), 10000U);
    if (ok == 0U)
    {
        printf("Weather SSL request failed, try TCP fallback\r\n");
        ok = AppESP8266_HTTPGet(APP_WEATHER_HOST, path, 0U, weather_http_buf, sizeof(weather_http_buf), 10000U);
    }

    if (ok == 0U)
    {
        weather_data.conn_fail = 1U;
        return 0U;
    }

    return AppWeather_Parse(weather_http_buf);
}

AppWeather_Data_t* AppWeather_Get(void)
{
    return &weather_data;
}

static uint8_t AppWeather_Parse(const char *response)
{
    const char *json;
    int temperature;

    weather_data.valid = 0U;
    if (response == 0)
    {
        return 0U;
    }

    json = strstr(response, "\"results\"");
    if (json == 0)
    {
        json = response;
    }

    if (Weather_ParseStringField(json, "\"name\":\"", weather_data.city, sizeof(weather_data.city)) == 0U)
    {
        return 0U;
    }

    if (Weather_ParseStringField(json, "\"text\":\"", weather_data.weather, sizeof(weather_data.weather)) == 0U)
    {
        return 0U;
    }

    if (Weather_ParseTemperature(json, &temperature) == 0U)
    {
        return 0U;
    }

    weather_data.temperature = (int16_t)temperature;
    weather_data.humidity = -1;
    weather_data.valid = 1U;
    printf("Weather parsed: city=%s, text=%s, temp=%d\r\n",
           weather_data.city,
           weather_data.weather,
           weather_data.temperature);

    return 1U;
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
