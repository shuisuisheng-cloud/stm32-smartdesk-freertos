#ifndef __APP_SENSOR_H__
#define __APP_SENSOR_H__

#include "main.h"

#include <stdint.h>

#define APP_GAS_USE_ADC 0
#define APP_GAS_USE_DO 0
#define APP_GAS_SIM_ALARM_TEST 0
#define APP_GAS_ALARM_THRESHOLD_RAW 2500U
#define APP_GAS_DO_ACTIVE_LEVEL GPIO_PIN_RESET

void AppSensor_Init(void);
uint16_t AppSensor_ReadLightAdc(void);
uint16_t AppSensor_ReadGasRaw(void);
void AppSensor_UpdateData(void);

#endif /* __APP_SENSOR_H__ */
