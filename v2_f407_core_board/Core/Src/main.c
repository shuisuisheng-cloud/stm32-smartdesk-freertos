/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "board_config.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DHT11_SAMPLE_PERIOD_MS    2000U
#define UART2_RX_BUFFER_SIZE    64U
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim6;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
static uint8_t uart2_rx_byte = 0U;
static char uart2_rx_buffer[UART2_RX_BUFFER_SIZE];

static volatile uint16_t uart2_rx_index = 0U;
static volatile uint8_t uart2_line_ready = 0U;
static volatile uint8_t uart2_line_invalid = 0U;
static volatile uint32_t uart2_error_code = HAL_UART_ERROR_NONE;
static volatile uint8_t uart2_error_pending = 0U;
static volatile uint8_t uart2_rx_restart_failed = 0U;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM6_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static int8_t dht11_failed_bit;
int fputc(int ch, FILE *f)
{
    uint8_t data = (uint8_t)ch;

    (void)f;

    HAL_UART_Transmit(BOARD_DEBUG_UART_HANDLE,
                      &data,
                      1,
                      HAL_MAX_DELAY);

    return ch;
}
static void delay_us(uint16_t time){
	__HAL_TIM_SET_COUNTER(&htim6,0);
	while (__HAL_TIM_GET_COUNTER(&htim6) < time){
		}
	
	}
static void DHT11_Data_SetOutput(void){
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin=BOARD_DHT11_GPIO_PIN;
	GPIO_InitStruct.Mode=GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull=GPIO_NOPULL;
	GPIO_InitStruct.Speed=GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(BOARD_DHT11_GPIO_PORT,&GPIO_InitStruct);
}
static void DHT11_Data_SetInput(void){
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin=BOARD_DHT11_GPIO_PIN;
	GPIO_InitStruct.Mode=GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull=GPIO_NOPULL;
	HAL_GPIO_Init(BOARD_DHT11_GPIO_PORT,&GPIO_InitStruct);
}
static uint8_t DHT11_WaitForLevel(
    GPIO_PinState target_level,
    uint16_t timeout_us
){
	__HAL_TIM_SET_COUNTER(&htim6,0);
	while  (HAL_GPIO_ReadPin(BOARD_DHT11_GPIO_PORT,BOARD_DHT11_GPIO_PIN)!=target_level){
		if (__HAL_TIM_GET_COUNTER(&htim6)>=timeout_us){
			return 0;}
		}
	return 1;
	}
static uint8_t DHT11_CheckResponse(void){
	DHT11_Data_SetOutput();
	HAL_GPIO_WritePin(BOARD_DHT11_GPIO_PORT,BOARD_DHT11_GPIO_PIN,GPIO_PIN_RESET);
	HAL_Delay(20);
	HAL_GPIO_WritePin(BOARD_DHT11_GPIO_PORT,BOARD_DHT11_GPIO_PIN,GPIO_PIN_SET);
	delay_us(30);
	DHT11_Data_SetInput();
	if (!DHT11_WaitForLevel(GPIO_PIN_RESET,100)){
		return 0;}
	if (!DHT11_WaitForLevel(GPIO_PIN_SET,100)){
		return 0;}
	if (!DHT11_WaitForLevel(GPIO_PIN_RESET,100)){
		return 0;}
	return 1;
}
static int8_t DHT11_ReadBit(void){
	GPIO_PinState sampled_level;
	if(!DHT11_WaitForLevel(GPIO_PIN_SET,100)){
		return -1;}
	delay_us(40);
	sampled_level=HAL_GPIO_ReadPin(BOARD_DHT11_GPIO_PORT,BOARD_DHT11_GPIO_PIN);
	if(!DHT11_WaitForLevel(GPIO_PIN_RESET,100)){
		return -1;}
	if(sampled_level==GPIO_PIN_SET){
		return 1;}
	else{
		return 0;}
}
static uint8_t DHT11_ReadData(uint8_t data[5]){
	uint8_t i = 0U;
	uint8_t byte_index;
	uint8_t bit_index;
	int8_t level;
	dht11_failed_bit = -1;
	for (uint8_t j = 0; j < 5; j++)
	{
    data[j] = 0U;
	}
	while(i<40){
		byte_index = i / 8;
		bit_index = 7 - (i % 8);
		level=DHT11_ReadBit();
		if (level==-1){
			dht11_failed_bit=(int)i;
			return 0;}
		if (level==1){
			data[byte_index] |= (1U << bit_index);
		}
		i=i+1;
	}
	return  1;
}
static uint8_t DHT11_ChecksumIsValid(const uint8_t data[5]){
	uint8_t check_data=data[0]+data[1]+data[2]+data[3];
	if (check_data==data[4]){
		return 1;
	}
	return 0;
}
static uint8_t Board_LED_SetAndVerify(GPIO_PinState target_level)
{
    HAL_GPIO_WritePin(BOARD_LED_GPIO_PORT,
                      BOARD_LED_GPIO_PIN,
                      target_level);

    if (HAL_GPIO_ReadPin(BOARD_LED_GPIO_PORT,
                         BOARD_LED_GPIO_PIN) == target_level)
    {
        return 1U;
    }

    return 0U;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	GPIO_PinState key_s1_status = BOARD_KEY_INACTIVE_LEVEL;
	GPIO_PinState last_key_s1_status=BOARD_KEY_INACTIVE_LEVEL;
	GPIO_PinState waiting_s1_status=BOARD_KEY_INACTIVE_LEVEL;
	uint32_t start_time = 0U;
	uint32_t last_dht11_sample_time=0U;
	uint8_t dht11_data[5] = {0};
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_TIM6_Init();
  /* USER CODE BEGIN 2 */
	printf("board:%s\r\n",BOARD_NAME);
	printf("STM32 Environment Terminal V2 boot OK\r\n");
	printf("DHT11 idle level: %d\r\n",
       HAL_GPIO_ReadPin(BOARD_DHT11_GPIO_PORT,
                        BOARD_DHT11_GPIO_PIN));
	HAL_TIM_Base_Start(&htim6);
	if (HAL_UART_Receive_IT(&huart2, &uart2_rx_byte, 1U) != HAL_OK)
{
    Error_Handler();
}
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		//printf("count = %lu\r\n", (unsigned long)count);
    //count++;
		//HAL_GPIO_TogglePin(BOARD_LED_GPIO_PORT, BOARD_LED_GPIO_PIN);
		key_s1_status=HAL_GPIO_ReadPin(BOARD_KEY_GPIO_PORT,BOARD_KEY_GPIO_PIN);
		if (waiting_s1_status != key_s1_status){
				start_time=HAL_GetTick();
				waiting_s1_status = key_s1_status;
			}
			if ( HAL_GetTick()-start_time>=20U && waiting_s1_status !=last_key_s1_status){
				last_key_s1_status=waiting_s1_status;
				if (last_key_s1_status ==BOARD_KEY_ACTIVE_LEVEL){
					HAL_GPIO_TogglePin(BOARD_LED_GPIO_PORT,BOARD_LED_GPIO_PIN);
					printf("KEY PRESSED\r\n");
				}
				else if (last_key_s1_status==BOARD_KEY_INACTIVE_LEVEL){
					printf("KEY RELEASED\r\n");}
				
			}
		uint32_t now=HAL_GetTick();
		if (((uint32_t)now-last_dht11_sample_time)>=DHT11_SAMPLE_PERIOD_MS){
				last_dht11_sample_time=now;
				if (DHT11_CheckResponse()){
					if (!DHT11_ReadData(dht11_data))
				{
							printf("DHT11 read timeout\r\n");
							printf("%u\r\n",dht11_failed_bit);
				}
				else if (!DHT11_ChecksumIsValid(dht11_data))
				{	
							printf("DHT11 checksum error\r\n");
				}
				else
				{
							printf("DHT11 raw: %u %u %u %u %u\r\n",(unsigned int)dht11_data[0],(unsigned int)dht11_data[1],(unsigned int)dht11_data[2],(unsigned int)dht11_data[3],(unsigned int)dht11_data[4]);
							printf("temperature:%u.%u\r\n",dht11_data[2],dht11_data[3]);
				}
				}
				else{
					printf("DHT11 response: TIMEOUT\r\n");
						}
			}
		if (uart2_line_ready==1U){
			printf("rx:%s\r\n",uart2_rx_buffer);
			if (strcmp(uart2_rx_buffer, "led_on") == 0)
{
    if (Board_LED_SetAndVerify(BOARD_LED_ACTIVE_LEVEL) == 1U)
    {
        printf("ack:led_on:success\r\n");
    }
    else
    {
        printf("ack:led_on:failed\r\n");
    }
}
else if (strcmp(uart2_rx_buffer, "led_off") == 0)
{
    if (Board_LED_SetAndVerify(BOARD_LED_INACTIVE_LEVEL) == 1U)
    {
        printf("ack:led_off:success\r\n");
    }
    else
    {
        printf("ack:led_off:failed\r\n");
    }
}
else
{
    printf("ack:%s:failed\r\n", uart2_rx_buffer);
}
			uart2_rx_index = 0U;
			uart2_line_ready = 0U;
		}
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
   }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 15;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 65535;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(DHT11_DATA_GPIO_Port, DHT11_DATA_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_D2_GPIO_Port, LED_D2_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : DHT11_DATA_Pin */
  GPIO_InitStruct.Pin = DHT11_DATA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DHT11_DATA_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : KEY_S1_Pin */
  GPIO_InitStruct.Pin = KEY_S1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(KEY_S1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LED_D2_Pin */
  GPIO_InitStruct.Pin = LED_D2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_D2_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
				 if (uart2_line_ready == 0U)
        {
            if (uart2_rx_byte == '\r')
            {}
            else if (uart2_rx_byte == '\n')
            {
								 if (uart2_line_invalid==1U){
										uart2_rx_index=0U;
									  uart2_line_invalid=0U;
								}
								 else if (uart2_rx_index > 0U)
                {
                    uart2_rx_buffer[uart2_rx_index]='\0';
                    uart2_line_ready=1U;
                }
								else{}
            }
            else
            {
							if (uart2_line_invalid==1U){
								}
								else
									{
										if (uart2_rx_byte >= 0x20U && uart2_rx_byte <= 0x7EU){
												if (uart2_rx_index < UART2_RX_BUFFER_SIZE - 1U)
											{
												uart2_rx_buffer[uart2_rx_index]=uart2_rx_byte;
												uart2_rx_index++;
											}
											else{
											uart2_line_invalid=1U;
												uart2_rx_index=0U;
											}
										}
										else{
											uart2_line_invalid=1U;
											uart2_rx_index=0U;}
								}
    }
						HAL_UART_Receive_IT(huart, &uart2_rx_byte, 1U);
	}
}
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart){
	uint32_t current_error;
	HAL_StatusTypeDef restart_status;
	if(huart->Instance == USART2){
		current_error|=huart->ErrorCode;
		uart2_error_code=uart2_error_code|current_error;
		uart2_error_pending=1U;
		uart2_rx_index=0U;
		uart2_line_ready=0U;
		uart2_rx_buffer[0]='\0';
		uart2_line_invalid=1U;
		if ((current_error&HAL_UART_ERROR_ORE)!= 0U){
			restart_status=HAL_UART_Receive_IT(huart,&uart2_rx_byte,1U);
			if((restart_status!=HAL_OK)){
				uart2_rx_restart_failed=1U;
			}
		}
	}
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
