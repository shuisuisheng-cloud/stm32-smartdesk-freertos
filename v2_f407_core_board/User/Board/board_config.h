#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "main.h"

/* USART2 is defined in main.c. */
extern UART_HandleTypeDef huart2;

/* Board information */
#define BOARD_NAME                 "STM32F407VET6_CORE_BOARD_V2"

/* On-board LED D2: PA1, active low */
#define BOARD_LED_GPIO_PORT        LED_D2_GPIO_Port
#define BOARD_LED_GPIO_PIN         LED_D2_Pin
#define BOARD_LED_ACTIVE_LEVEL     GPIO_PIN_RESET
#define BOARD_LED_INACTIVE_LEVEL   GPIO_PIN_SET
#define BOARD_KEY_GPIO_PORT        KEY_S1_GPIO_Port
#define BOARD_KEY_GPIO_PIN         KEY_S1_Pin
#define BOARD_KEY_ACTIVE_LEVEL    GPIO_PIN_SET
#define BOARD_KEY_INACTIVE_LEVEL  GPIO_PIN_RESET
#define BOARD_DHT11_GPIO_PORT     DHT11_DATA_GPIO_Port
#define BOARD_DHT11_GPIO_PIN      DHT11_DATA_Pin

/* Debug UART: USART2 */
#define BOARD_DEBUG_UART_HANDLE    (&huart2)

#endif
