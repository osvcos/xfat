#ifndef DATETIME_H
#define DATETIME_H

#include <time.h>

#include "types.h"

/* 
 * This structure holds date/time fields in conformance
 * with the FAT specification
 */
struct fat_datetime {
    u32 day;
    u32 month;
    u32 year;
    u32 seconds;
    u32 minutes;
    u32 hours;
};

/*
 * Stores date and time information into a
 * struct fat_datetime structure, according to the
 * format dictated by the FAT specs.
 * 
 * @dt: a reference to a struct fat_datetime variable
 * that will contain the new date and time information.
 */
s32 fat_getdatetime(struct fat_datetime *dt);

/*
 * Gets the time field of the directory structure
 */
u16 fat_gettime(struct fat_datetime *dt);

/*
 * Gets the date field of the directory structure
 */
u16 fat_getdate(struct fat_datetime *dt);

/*
 * Takes tow parameters, date_field and optionally
 * time_field, and the it returns a struct tm in the tms
 * argument. The date_field and time_field are encode in the
 * FAT date/time format.
 * 
 * @date_field: the date encoded eccording the FAT specification.
 * 
 * @time_field: the time encoded according the FAT specification.
 * 
 * @tms: this parameter holds the the broken down time representation
 * of date_field and time_field.
 */
void get_tm(u16 date_field, u16 time_field, struct tm *tms);

#endif
