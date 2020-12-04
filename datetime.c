#include <string.h>

#include "datetime.h"

s32 fat_getdatetime(struct fat_datetime *dt)
{
    struct tm dts;
    time_t rawtime;
    
    if(time(&rawtime) == -1)
        return -1;
    
    if(localtime_r(&rawtime, &dts) == NULL)
        return -1;
    
    dt->seconds   = ((dts.tm_sec - 1) / 2) - 1;
    dt->minutes   = dts.tm_min;
    dt->hours     = dts.tm_hour;
    
    while(dt->seconds > 29)
        --dt->seconds;
    
    dt->day   = dts.tm_mday;
    dt->month = dts.tm_mon + 1;
    dt->year  = dts.tm_year - 80;
    
    return 0;
}

u16 fat_gettime(struct fat_datetime *dt)
{
    u16 time = 0;
    
    time |= dt->seconds;
    time |= dt->minutes << 5;
    time |= dt->hours << 11;
    return time;
}

u16 fat_getdate(struct fat_datetime *dt)
{
    u16 date = 0;
    
    date |= (dt->day);
    date |= (dt->month << 5);
    date |= (dt->year << 9);
    return date;
}

void get_tm(u16 date_field, u16 time_field, struct tm *tms)
{
    tms->tm_isdst = -1;
    tms->tm_mday  = date_field & 0x001F;
    tms->tm_mon   = (((date_field & 0x01E0) >> 5) - 1);
    tms->tm_year  = (((date_field & 0xFE00) >> 9) + 80);
    
    if(time == 0)
        return;
    
    tms->tm_sec  = ((time_field & 0x001F) * 2);
    tms->tm_min  = ((time_field & 0x07E0) >> 5);
    tms->tm_hour = ((time_field & 0xF800) >> 11);
}
