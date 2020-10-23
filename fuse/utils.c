#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "utils.h"
#include "xfat.h"

static s32 get_tm(u16 date_field, u16 time_field, struct tm *tms)
{
    struct tm t;
    time_t raw;
    
    memset(&t, 0, sizeof(struct tm));
    
    time(&raw);
    localtime_r(&raw, &t);
    
    tms->tm_mday = date_field & 0x001F;
    tms->tm_mon  = (((date_field & 0x01E0) >> 5) - 1);
    tms->tm_year = (((date_field & 0xFE00) >> 9) + 80);
    
    if(time == 0)
        return 0;
    
    tms->tm_sec  = time_field & 0x001F;
    tms->tm_min  = ((time_field & 0x07E0) >> 5);
    tms->tm_hour = ((time_field & 0xF800) >> 11);
    
    if(t.tm_isdst)
        tms->tm_hour -= 1;
    
    return 0;
}

u32 get_cluster32(u16 hi, u16 low)
{
    return ((hi << 16) | low);
}

s32 get_stat_from_directory(Directory *dir, struct stat *st)
{
    struct tm tm_atime;
    struct tm tm_mtime;
    struct timespec atime;
    struct timespec mtime;
    
    memset(st, 0, sizeof(struct stat));
    memset(&tm_atime, 0, sizeof(struct tm));
    memset(&tm_mtime, 0, sizeof(struct tm));
    memset(&atime, 0, sizeof(struct timespec));
    memset(&mtime, 0, sizeof(struct timespec));
    
    st->st_size    = dir->file_size;
    st->st_uid     = getuid();
    st->st_gid     = getgid();
    
    if(dir->attributes & ATTR_DIRECTORY || dir->attributes == ATTR_VOLUME_ID)
        st->st_mode = S_IFDIR | 0755;
    if(dir->attributes & ATTR_ARCHIVE)
        st->st_mode = S_IFREG | 0644;
    
    st->st_blksize = get_cluster_size();
    
    get_tm(dir->last_access_date, 0, &tm_atime);
    get_tm(dir->last_mod_date, dir->last_mod_time, &tm_mtime);
    
    st->st_atime = mktime(&tm_atime);
    st->st_mtime = mktime(&tm_mtime);
    
    return 0;
}

s32 lookup_short_entry(const char *path, u32 *starting_cluster, Directory *dir)
{
    char *new_path      = NULL;
    char pretty_name[13];
    u32 current_cluster = get_root_cluster();
    u32 offset          = 0;
    u32 ret             = 0;
    char *token         = NULL;
    char *intbuff       = NULL;
    Directory directory;
    
    new_path = malloc(strlen(path + 1) + 1);
    memset(new_path, 0, strlen(path + 1) + 1);
    memcpy(new_path, path + 1, strlen(path + 1));
    memset(&directory, 0, sizeof(Directory));
    
    token = strtok_r(new_path, "/", &intbuff);
    
    while(token != NULL)
    {
        printf("lookup_short_entry: looking for token %s\n", token);
        
        while(get_directory_entry(&current_cluster, &directory, &offset) != -1)
        {
            prettify_83_name(directory.name, pretty_name);
            
            if(strncmp(token, pretty_name, 12) == 0)
            {
                printf("lookup_short_entry: foud %s\n", token);
                
                current_cluster = get_cluster32(directory.first_clus_hi,
                                                directory.first_clus_low);
                offset = 0;
                token = strtok_r(NULL, "/", &intbuff);
                
                if(token == NULL)
                {
                    ret = 0;
                    goto  leave;
                }
            }
        }
        
        ret = -1;
        goto leave;
    }
    
    ret = 0;
leave:
    free(new_path);
    *starting_cluster = current_cluster;
    
    if(dir != NULL)
        memcpy(dir, &directory, sizeof(Directory));
    
    return ret;
}

s32 prettify_83_name(u8 *input_name, u8 *output_name)
{
    memset(output_name, 0, 13);
    
    memcpy(output_name, input_name, 8);
    
    if(input_name[8] != 0x20
        || input_name[9] != 0x20
        || input_name[10] != 0x20)
    {
        strncpy(output_name + 8, ".", 1);
        memcpy(output_name + 9, input_name + 8, 3);
    }
}

