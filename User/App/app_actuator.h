#ifndef __APP_ACTUATOR_H__
#define __APP_ACTUATOR_H__

#include "main.h"

#include <stdint.h>

#define APP_BUZZER_ENABLE 0
#define APP_BUZZER_GPIO_PORT GPIOB
#define APP_BUZZER_GPIO_PIN GPIO_PIN_0
#define APP_BUZZER_ACTIVE_LEVEL GPIO_PIN_SET

/*
 * STM32 GPIO cannot drive a motor directly. Use a MOSFET, motor driver module,
 * relay module, or fan module with proper power and flyback protection.
 */
#define APP_FAN_ENABLE 0
#define APP_FAN_USE_PWM 0
#define APP_FAN_GPIO_PORT GPIOB
#define APP_FAN_GPIO_PIN GPIO_PIN_1
#define APP_FAN_ACTIVE_LEVEL GPIO_PIN_SET

void AppActuator_Init(void);
void AppActuator_Update(void);
void AppActuator_BuzzerOn(void);
void AppActuator_BuzzerOff(void);
void AppActuator_BuzzerToggle(void);
void AppActuator_FanOn(void);
void AppActuator_FanOff(void);
void AppActuator_FanSetSpeed(uint8_t percent);
void AppActuator_FanToggle(void);

#endif /* __APP_ACTUATOR_H__ */
