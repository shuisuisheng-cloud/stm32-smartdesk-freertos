#include "app_sensor.h"

#include "app_data.h"
#include "main.h"

extern ADC_HandleTypeDef hadc1;

void AppSensor_Init(void)
{
    AppSensor_UpdateData();
}

uint16_t AppSensor_ReadLightAdc(void)
{
    uint16_t value = 0U;

    if (HAL_ADC_Start(&hadc1) == HAL_OK)
    {
        if (HAL_ADC_PollForConversion(&hadc1, 10U) == HAL_OK)
        {
            value = (uint16_t)HAL_ADC_GetValue(&hadc1);
        }

        HAL_ADC_Stop(&hadc1);
    }

    return value;
}

void AppSensor_UpdateData(void)
{
    SmartDesk_Data_t *data = AppData_Get();

    data->light_adc = AppSensor_ReadLightAdc();
    data->comfort_score = AppData_CalcComfortScore(data);
}
