#ifndef __APP_WEATHER_H__
#define __APP_WEATHER_H__

#include <stdint.h>

typedef struct
{
    char city[24];
    char weather[24];
    int16_t temperature;
    int16_t humidity;
    uint8_t valid;
    uint8_t conn_fail;
} AppWeather_Data_t;

void AppWeather_Init(void);
uint8_t AppWeather_UpdateFromESP8266(void);
AppWeather_Data_t* AppWeather_Get(void);

#endif /* __APP_WEATHER_H__ */
