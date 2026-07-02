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

/* Debug UART: USART2 */
#define BOARD_DEBUG_UART_HANDLE    (&huart2)

#endif
