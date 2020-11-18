#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "datetime.h"
#include "utils.h"
#include "xfat.h"

void get_stat_from_directory(Directory *dir, struct stat *st)
{
    struct tm tm_atime;
    struct tm tm_mtime;
    
    memset(&tm_atime, 0, sizeof(struct tm));
    memset(&tm_mtime, 0, sizeof(struct tm));
    
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
}

s32 lookup_entry(const char *path, u32 *starting_cluster, dir_info *di)
{
    char *new_path      = NULL;
    u32 current_cluster = get_root_cluster();
    u32 offset          = 0;
    s32 ret             = 0;
    char *token         = NULL;
    char *intbuff       = NULL;
    dir_info dinfo;
    
    new_path = malloc(strlen(path + 1) + 1);
    memset(new_path, 0, strlen(path + 1) + 1);
    memcpy(new_path, path + 1, strlen(path + 1));
    memset(&dinfo, 0, sizeof(dir_info));
    
    token = strtok_r(new_path, "/", &intbuff);
    
    while(token != NULL)
    {
        printf("lookup_entry: looking for token %s\n", token);
        
        while(get_directory_entry(&current_cluster, &dinfo, &offset) != -1)
        {
            if(strncmp(token, dinfo.long_name, MAX_LFN_LENGTH) == 0)
            {
                printf("lookup_entry: found %s\n", token);
                
                current_cluster = dinfo.cluster32;
                offset = 0;
                token = strtok_r(NULL, "/", &intbuff);
                
                if(token == NULL)
                    goto  leave;
            }
        }
        
        ret = -1;
        goto leave;
    }
    
leave:
    free(new_path);
    *starting_cluster = current_cluster;
    
    if(di != NULL)
        memcpy(di, &dinfo, sizeof(dir_info));
    
    return ret;
}

