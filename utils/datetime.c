#include <stdio.h>
#include <string.h>

#include "datetime.h"
#include "xfat.h"

void print_datetime_info(struct fat_datetime dt);

void print_datetime_info(struct fat_datetime dt)
{
    printf("Day:      %u (%x)\n", dt.day, dt.day);
    printf("Month:    %u (%x)\n", dt.month, dt.month);
    printf("Year:     %u (%x)\n", dt.year, dt.year);
    printf("FAT date: %x\n", fat_getdate(&dt));
    printf("\n");
    printf("Seconds:    %u (%x)\n", dt.seconds, dt.seconds);
    printf("Minutes:    %u (%x)\n", dt.minutes, dt.minutes);
    printf("Hours:      %u (%x)\n", dt.hours, dt.hours);
    printf("FAT time: %x\n", fat_gettime(&dt));
}

int main(int argc, char *argv[])
{
    struct fat_datetime dt;
    struct tm tms;
    u16 date = 0;
    u16 time = 0;
    
    memset(&dt, 0, sizeof(struct fat_datetime));
    memset(&tms, 0, sizeof(struct tm));
    
    if(fat_getdatetime(&dt) == -1)
    {
        printf("fat_getdatetime() failed\n");
        return 1;
    }
    
    printf("after calling fat_getdatetime()\n");
    printf("===========\n");
    print_datetime_info(dt);
    printf("\n");
    
    date = fat_getdate(&dt);
    time = fat_gettime(&dt);
    
    get_tm(date, time, &tms);
    
    printf("after calling get_tm()\n");
    printf("===========\n");
    
    printf("min:  %u\n", tms.tm_min);
    printf("sec:  %u\n", tms.tm_sec);
    printf("hour: %u\n", tms.tm_hour);
    printf("\n");
    printf("day:  %u\n", tms.tm_mday);
    printf("mon:  %u\n", tms.tm_mon);
    printf("year: %u\n", tms.tm_year);
    
    return 0;
}
