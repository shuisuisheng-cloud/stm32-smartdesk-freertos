#include "app_voice.h"

#include "main.h"

#if APP_VOICE_ENABLE && APP_VOICE_USE_I2C
extern I2C_HandleTypeDef hi2c1;
#endif

static uint8_t voice_last_command = APP_VOICE_CMD_NONE;

void AppVoice_Init(void)
{
    voice_last_command = APP_VOICE_CMD_NONE;
}

void AppVoice_Update(void)
{
#if APP_VOICE_ENABLE
#if APP_VOICE_USE_I2C
    /*
     * TODO: Add DF2301Q/SEN0539 command ID polling over I2C address 0x64.
     * Keep this non-blocking and avoid long I2C timeouts. If UART is used later,
     * use a new UART instance and do not share USART1 with ESP8266.
     */
    (void)hi2c1;
#endif
#endif
}

uint8_t AppVoice_GetLastCommand(void)
{
    return voice_last_command;
}

void AppVoice_ClearCommand(void)
{
    voice_last_command = APP_VOICE_CMD_NONE;
}
