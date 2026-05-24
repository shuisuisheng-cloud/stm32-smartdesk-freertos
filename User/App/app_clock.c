#include "app_clock.h"

#include "app_data.h"
#include "main.h"

#define APP_CLOCK_DAY_SECONDS 86400UL

static uint32_t clock_start_tick = 0U;
static AppClock_Time_t current_time;
static uint8_t alarm_hour = 0U;
static uint8_t alarm_minute = 1U;
static uint8_t alarm_triggered = 0U;

static AppClock_Time_t AppClock_SecondsToTime(uint32_t seconds);

void AppClock_Init(void)
{
    clock_start_tick = HAL_GetTick();
    current_time.hour = 0U;
    current_time.minute = 0U;
    current_time.second = 0U;
    AppClock_SetAlarm(0U, 1U);
    AppClock_ClearAlarm();
}

void AppClock_Update(void)
{
    uint32_t elapsed_seconds = ((HAL_GetTick() - clock_start_tick) / 1000U) % APP_CLOCK_DAY_SECONDS;
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

static AppClock_Time_t AppClock_SecondsToTime(uint32_t seconds)
{
    AppClock_Time_t time;

    time.hour = (uint8_t)(seconds / 3600U);
    seconds %= 3600U;
    time.minute = (uint8_t)(seconds / 60U);
    time.second = (uint8_t)(seconds % 60U);

    return time;
}
