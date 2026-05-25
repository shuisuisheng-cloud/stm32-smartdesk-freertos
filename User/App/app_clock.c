#include "app_clock.h"

#include "app_data.h"
#include "main.h"

#include <string.h>

#define APP_CLOCK_DAY_SECONDS 86400UL

static uint32_t clock_base_tick = 0U;
static uint32_t clock_base_seconds = 0U;
static AppClock_Time_t current_time;
static uint8_t alarm_hour = 0U;
static uint8_t alarm_minute = 1U;
static uint8_t alarm_triggered = 0U;
static uint8_t time_synced = 0U;

static AppClock_Time_t AppClock_SecondsToTime(uint32_t seconds);
static uint8_t AppClock_ParseTimeField(const char *s, uint8_t *hour, uint8_t *minute, uint8_t *second);

void AppClock_Init(void)
{
    clock_base_tick = HAL_GetTick();
    clock_base_seconds = 0U;
    current_time.hour = 0U;
    current_time.minute = 0U;
    current_time.second = 0U;
    time_synced = 0U;
    AppData_Get()->time_synced = 0U;
    AppClock_SetAlarm(0U, 1U);
    AppClock_ClearAlarm();
}

void AppClock_Update(void)
{
    uint32_t elapsed_seconds = (clock_base_seconds + ((HAL_GetTick() - clock_base_tick) / 1000U)) % APP_CLOCK_DAY_SECONDS;
    uint32_t alarm_seconds = ((uint32_t)alarm_hour * 3600U) + ((uint32_t)alarm_minute * 60U);

    current_time = AppClock_SecondsToTime(elapsed_seconds);

    if ((alarm_triggered == 0U) && (elapsed_seconds >= alarm_seconds))
    {
        alarm_triggered = 1U;
    }

    if (alarm_triggered != 0U)
    {
        AppData_Get()->alarm_on = 1U;
    }
}

AppClock_Time_t AppClock_GetTime(void)
{
    return current_time;
}

void AppClock_SetAlarm(uint8_t hour, uint8_t minute)
{
    alarm_hour = hour % 24U;
    alarm_minute = minute % 60U;
}

uint8_t AppClock_IsAlarmTriggered(void)
{
    return alarm_triggered;
}

void AppClock_ClearAlarm(void)
{
    alarm_triggered = 0U;
    AppData_Get()->alarm_on = 0U;
}

uint8_t AppClock_SetTimeFromSNTPString(const char *sntp_str)
{
    const char *payload;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;

    if (sntp_str == 0)
    {
        time_synced = 0U;
        return 0U;
    }

    payload = strstr(sntp_str, "+CIPSNTPTIME:");
    if (payload == 0)
    {
        time_synced = 0U;
        return 0U;
    }

    if ((strstr(payload, "1970") != 0) ||
        (strstr(payload, "Jan 1") != 0) ||
        (strstr(payload, "Jan 01") != 0))
    {
        time_synced = 0U;
        return 0U;
    }

    if (AppClock_ParseTimeField(payload, &hour, &minute, &second) == 0U)
    {
        time_synced = 0U;
        return 0U;
    }

    current_time.hour = hour;
    current_time.minute = minute;
    current_time.second = second;
    clock_base_seconds = ((uint32_t)hour * 3600U) + ((uint32_t)minute * 60U) + second;
    clock_base_tick = HAL_GetTick();
    time_synced = 1U;

    return 1U;
}

uint8_t AppClock_IsTimeSynced(void)
{
    return time_synced;
}

static AppClock_Time_t AppClock_SecondsToTime(uint32_t seconds)
{
    AppClock_Time_t time;

    time.hour = (uint8_t)(seconds / 3600U);
    seconds %= 3600U;
    time.minute = (uint8_t)(seconds / 60U);
    time.second = (uint8_t)(seconds % 60U);

    return time;
}

static uint8_t AppClock_ParseTimeField(const char *s, uint8_t *hour, uint8_t *minute, uint8_t *second)
{
    const char *p = s;

    while (*p != '\0')
    {
        if ((p[0] >= '0') && (p[0] <= '2') &&
            (p[1] >= '0') && (p[1] <= '9') &&
            (p[2] == ':') &&
            (p[3] >= '0') && (p[3] <= '5') &&
            (p[4] >= '0') && (p[4] <= '9') &&
            (p[5] == ':') &&
            (p[6] >= '0') && (p[6] <= '5') &&
            (p[7] >= '0') && (p[7] <= '9'))
        {
            *hour = (uint8_t)(((p[0] - '0') * 10) + (p[1] - '0'));
            *minute = (uint8_t)(((p[3] - '0') * 10) + (p[4] - '0'));
            *second = (uint8_t)(((p[6] - '0') * 10) + (p[7] - '0'));

            if (*hour < 24U)
            {
                return 1U;
            }
        }

        p++;
    }

    return 0U;
}
