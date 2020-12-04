#ifndef DATETIME_H
#define DATETIME_H

#include <time.h>

#include "types.h"

struct fat_datetime {
    u32 day;
    u32 month;
    u32 year;
    u32 seconds;
    u32 minutes;
    u32 hours;
};

s32 fat_getdatetime(struct fat_datetime *dt);
u16 fat_gettime(struct fat_datetime *dt);
u16 fat_getdate(struct fat_datetime *dt);
void get_tm(u16 date_field, u16 time_field, struct tm *tms);

#endif
