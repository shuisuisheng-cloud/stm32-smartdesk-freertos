#ifndef __APP_SENSOR_H__
#define __APP_SENSOR_H__

#include <stdint.h>

void AppSensor_Init(void);
uint16_t AppSensor_ReadLightAdc(void);
void AppSensor_UpdateData(void);

#endif /* __APP_SENSOR_H__ */
