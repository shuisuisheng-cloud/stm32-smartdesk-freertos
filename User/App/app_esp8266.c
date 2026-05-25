#include "app_esp8266.h"

#include "main.h"

#include <string.h>

#define APP_ESP8266_RX_BUFFER_SIZE 128U

extern UART_HandleTypeDef huart1;

void AppESP8266_Init(void)
{
}

uint8_t AppESP8266_SendAT(const char *cmd, const char *expect, uint32_t timeout)
{
    uint8_t ch;
    char rx_buf[APP_ESP8266_RX_BUFFER_SIZE];
    uint16_t rx_len = 0U;
    uint32_t start_tick = HAL_GetTick();

    memset(rx_buf, 0, sizeof(rx_buf));

    if (HAL_UART_Transmit(&huart1, (uint8_t *)cmd, (uint16_t)strlen(cmd), 100U) != HAL_OK)
    {
        return 0U;
    }

    while ((HAL_GetTick() - start_tick) < timeout)
    {
        if (HAL_UART_Receive(&huart1, &ch, 1U, 10U) == HAL_OK)
        {
            if (rx_len < (APP_ESP8266_RX_BUFFER_SIZE - 1U))
            {
                rx_buf[rx_len] = (char)ch;
                rx_len++;
                rx_buf[rx_len] = '\0';
            }

            if (strstr(rx_buf, expect) != 0)
            {
                return 1U;
            }
        }
    }

    return 0U;
}

uint8_t AppESP8266_TestAT(void)
{
    return AppESP8266_SendAT("AT\r\n", "OK", 1000U);
}
