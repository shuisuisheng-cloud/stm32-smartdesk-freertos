#ifndef __APP_VOICE_H__
#define __APP_VOICE_H__

#include <stdint.h>

#define APP_VOICE_ENABLE 0
#define APP_VOICE_USE_I2C 0
#define APP_VOICE_I2C_ADDR 0x64U

typedef enum
{
    APP_VOICE_CMD_NONE = 0,
    APP_VOICE_CMD_PAGE_NEXT,
    APP_VOICE_CMD_MODE_AUTO,
    APP_VOICE_CMD_MODE_MANUAL,
    APP_VOICE_CMD_ALARM_OFF,
    APP_VOICE_CMD_WEATHER_PAGE
} AppVoice_Command_t;

void AppVoice_Init(void);
void AppVoice_Update(void);
uint8_t AppVoice_GetLastCommand(void);
void AppVoice_ClearCommand(void);

#endif /* __APP_VOICE_H__ */
