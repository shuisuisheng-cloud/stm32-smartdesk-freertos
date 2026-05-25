#include "app_clock.h"

#include "app_data.h"
#include "main.h"

#include <stdio.h>
#include <string.h>

#define APP_CLOCK_DAY_SECONDS 86400UL

static uint32_t clock_base_tick = 0U;
static uint32_t clock_base_seconds = 0U;
static uint16_t clock_base_year = 2000U;
static uint8_t clock_base_month = 1U;
static uint8_t clock_base_day = 1U;
static uint8_t clock_base_weekday = 6U;
static AppClock_DateTime_t current_time;
static uint8_t alarm_hour = 0U;
static uint8_t alarm_minute = 1U;
static uint8_t alarm_triggered = 0U;
static uint8_t time_synced = 0U;

static AppClock_DateTime_t AppClock_MakeDateTime(uint32_t day_offset, uint32_t seconds);
static uint8_t AppClock_ParseSNTP(const char *s, AppClock_DateTime_t *date_time);
static uint8_t AppClock_MonthFromName(const char *name);
static uint8_t AppClock_WeekdayFromName(const char *name);
static uint8_t AppClock_DaysInMonth(uint16_t year, uint8_t month);
static uint8_t AppClock_IsLeapYear(uint16_t year);

void AppClock_Init(void)
{
    clock_base_tick = HAL_GetTick();
    clock_base_seconds = 0U;
    clock_base_year = 2000U;
    clock_base_month = 1U;
    clock_base_day = 1U;
    clock_base_weekday = 6U;
    current_time.year = clock_base_year;
    current_time.month = clock_base_month;
    current_time.day = clock_base_day;
    current_time.weekday = clock_base_weekday;
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
    uint32_t total_seconds = clock_base_seconds + ((HAL_GetTick() - clock_base_tick) / 1000U);
    uint32_t day_offset = total_seconds / APP_CLOCK_DAY_SECONDS;
    uint32_t elapsed_seconds = total_seconds % APP_CLOCK_DAY_SECONDS;
    uint32_t alarm_seconds = ((uint32_t)alarm_hour * 3600U) + ((uint32_t)alarm_minute * 60U);

    current_time = AppClock_MakeDateTime(day_offset, elapsed_seconds);

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

AppClock_DateTime_t AppClock_GetDateTime(void)
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
    AppClock_DateTime_t date_time;

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

    if (strstr(payload, "1970") != 0)
    {
        time_synced = 0U;
        return 0U;
    }

    if (AppClock_ParseSNTP(payload, &date_time) == 0U)
    {
        time_synced = 0U;
        return 0U;
    }

    current_time = date_time;
    clock_base_year = date_time.year;
    clock_base_month = date_time.month;
    clock_base_day = date_time.day;
    clock_base_weekday = date_time.weekday;
    clock_base_seconds = ((uint32_t)date_time.hour * 3600U) + ((uint32_t)date_time.minute * 60U) + date_time.second;
    clock_base_tick = HAL_GetTick();
    time_synced = 1U;

    return 1U;
}

uint8_t AppClock_IsTimeSynced(void)
{
    return time_synced;
}

const char* AppClock_GetWeekdayName(uint8_t weekday)
{
    static const char *names[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

    if (weekday < 7U)
    {
        return names[weekday];
    }

    return "Unk";
}

static AppClock_DateTime_t AppClock_MakeDateTime(uint32_t day_offset, uint32_t seconds)
{
    AppClock_DateTime_t date_time;
    uint32_t i;

    date_time.year = clock_base_year;
    date_time.month = clock_base_month;
    date_time.day = clock_base_day;
    date_time.weekday = (uint8_t)((clock_base_weekday + day_offset) % 7U);

    for (i = 0U; i < day_offset; i++)
    {
        date_time.day++;
        if (date_time.day > AppClock_DaysInMonth(date_time.year, date_time.month))
        {
            date_time.day = 1U;
            date_time.month++;
            if (date_time.month > 12U)
            {
                date_time.month = 1U;
                date_time.year++;
            }
        }
    }

    date_time.hour = (uint8_t)(seconds / 3600U);
    seconds %= 3600U;
    date_time.minute = (uint8_t)(seconds / 60U);
    date_time.second = (uint8_t)(seconds % 60U);

    return date_time;
}

static uint8_t AppClock_ParseSNTP(const char *s, AppClock_DateTime_t *date_time)
{
    char weekday_name[4];
    char month_name[4];
    unsigned int day;
    unsigned int hour;
    unsigned int minute;
    unsigned int second;
    unsigned int year;
    uint8_t month;
    uint8_t weekday;

    if ((s == 0) || (date_time == 0))
    {
        return 0U;
    }

    if (sscanf(s, "+CIPSNTPTIME:%3s %3s %u %u:%u:%u %u",
               weekday_name,
               month_name,
               &day,
               &hour,
               &minute,
               &second,
               &year) != 7)
    {
        return 0U;
    }

    weekday_name[3] = '\0';
    month_name[3] = '\0';
    weekday = AppClock_WeekdayFromName(weekday_name);
    month = AppClock_MonthFromName(month_name);

    if ((year == 1970U) ||
        (year < 2000U) ||
        (month == 0U) ||
        (weekday > 6U) ||
        (day == 0U) ||
        (day > AppClock_DaysInMonth((uint16_t)year, month)) ||
        (hour >= 24U) ||
        (minute >= 60U) ||
        (second >= 60U))
    {
        return 0U;
    }

    date_time->year = (uint16_t)year;
    date_time->month = month;
    date_time->day = (uint8_t)day;
    date_time->weekday = weekday;
    date_time->hour = (uint8_t)hour;
    date_time->minute = (uint8_t)minute;
    date_time->second = (uint8_t)second;

    return 1U;
}

static uint8_t AppClock_MonthFromName(const char *name)
{
    static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    uint8_t i;

    for (i = 0U; i < 12U; i++)
    {
        if (strncmp(name, months[i], 3U) == 0)
        {
            return (uint8_t)(i + 1U);
        }
    }

    return 0U;
}

static uint8_t AppClock_WeekdayFromName(const char *name)
{
    static const char *weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    uint8_t i;

    for (i = 0U; i < 7U; i++)
    {
        if (strncmp(name, weekdays[i], 3U) == 0)
        {
            return i;
        }
    }

    return 0xFFU;
}

static uint8_t AppClock_DaysInMonth(uint16_t year, uint8_t month)
{
    static const uint8_t days[] = {31U, 28U, 31U, 30U, 31U, 30U,
                                   31U, 31U, 30U, 31U, 30U, 31U};

    if ((month == 0U) || (month > 12U))
    {
        return 31U;
    }

    if ((month == 2U) && (AppClock_IsLeapYear(year) != 0U))
    {
        return 29U;
    }

    return days[month - 1U];
}

static uint8_t AppClock_IsLeapYear(uint16_t year)
{
    if ((year % 400U) == 0U)
    {
        return 1U;
    }

    if ((year % 100U) == 0U)
    {
        return 0U;
    }

    return ((year % 4U) == 0U) ? 1U : 0U;
}
