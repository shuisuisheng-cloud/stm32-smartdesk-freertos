#ifndef __APP_ACTUATOR_H__
#define __APP_ACTUATOR_H__

#include "main.h"

#define APP_BUZZER_ENABLE 0
#define APP_BUZZER_GPIO_PORT GPIOB
#define APP_BUZZER_GPIO_PIN GPIO_PIN_0
#define APP_BUZZER_ACTIVE_LEVEL GPIO_PIN_SET

void AppActuator_Init(void);
void AppActuator_Update(void);
void AppActuator_BuzzerOn(void);
void AppActuator_BuzzerOff(void);
void AppActuator_BuzzerToggle(void);

#endif /* __APP_ACTUATOR_H__ */
