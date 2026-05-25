#ifndef __APP_CLOCK_H__
#define __APP_CLOCK_H__

#include <stdint.h>

typedef struct
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t weekday;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} AppClock_DateTime_t;

typedef AppClock_DateTime_t AppClock_Time_t;

void AppClock_Init(void);
void AppClock_Update(void);
AppClock_Time_t AppClock_GetTime(void);
AppClock_DateTime_t AppClock_GetDateTime(void);
void AppClock_SetAlarm(uint8_t hour, uint8_t minute);
uint8_t AppClock_IsAlarmTriggered(void);
void AppClock_ClearAlarm(void);
uint8_t AppClock_SetTimeFromSNTPString(const char *sntp_str);
uint8_t AppClock_IsTimeSynced(void);
const char* AppClock_GetWeekdayName(uint8_t weekday);

#endif /* __APP_CLOCK_H__ */
