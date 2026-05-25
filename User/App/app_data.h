#ifndef __APP_DATA_H__
#define __APP_DATA_H__

#include <stdint.h>

typedef struct
{
    float temperature;
    float humidity;
    uint16_t gas_adc;
    uint16_t light_adc;
    uint8_t fan_on;
    uint8_t light_on;
    uint8_t alarm_on;
    uint8_t mode;
    uint8_t comfort_score;
    uint8_t esp_ok;
    uint8_t wifi_ok;
} SmartDesk_Data_t;

void AppData_Init(void);
SmartDesk_Data_t* AppData_Get(void);
void AppData_UpdateFake(void);
void AppData_NextMode(void);
uint8_t AppData_CalcComfortScore(SmartDesk_Data_t *data);
const char* AppData_GetLightLevel(uint16_t light_adc);
const char* AppData_GetGasLevel(uint16_t gas_adc);
const char* AppData_GetModeName(uint8_t mode);

#endif /* __APP_DATA_H__ */
