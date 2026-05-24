#ifndef __APP_CLOCK_H__
#define __APP_CLOCK_H__

#include <stdint.h>

typedef struct
{
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} AppClock_Time_t;

void AppClock_Init(void);
void AppClock_Update(void);
AppClock_Time_t AppClock_GetTime(void);
void AppClock_SetAlarm(uint8_t hour, uint8_t minute);
uint8_t AppClock_IsAlarmTriggered(void);
void AppClock_ClearAlarm(void);

#endif /* __APP_CLOCK_H__ */
