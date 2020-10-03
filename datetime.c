#include <time.h>

#include "datetime.h"

s32 fat_getdatetime(struct fat_datetime *dt)
{
    struct tm *dts = NULL;
    time_t rawtime;
    
    if(time(&rawtime) == -1)
        return -1;
    
    dts = gmtime(&rawtime);
    
    dt->seconds   = ((dts->tm_sec + 1) / 2) - 1;
    dt->minutes   = dts->tm_min;
    dt->hours     = dts->tm_hour;
    
    while(dt->seconds > 29)
        --dt->seconds;
    
    dt->day   = dts->tm_mday;
    dt->month = dts->tm_mon + 1;
    dt->year  = dts->tm_year + 80;
    
    return 0;
}

u16 fat_gettime(struct fat_datetime *dt)
{
    u16 time = 0;
    time |= (dt->seconds << 12);
    time |= (dt->minutes << 6);
    time |= (dt->hours);
    return time;
}

u16 fat_getdate(struct fat_datetime *dt)
{
    u16 date = 0;
    date |= (dt->day << 12);
    date |= (dt->month << 8);
    date |= (dt->year);
    return date;
}
