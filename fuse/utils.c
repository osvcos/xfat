#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"
#include "xfat.h"

u32 get_cluster32(u16 hi, u16 low)
{
    return ((hi << 16) | low);
}

s32 get_stat_from_directory(Directory *dir, struct stat *st)
{
    memset(st, 0, sizeof(struct stat));
    
    st->st_size    = dir->file_size;
    st->st_uid     = getuid();
    st->st_gid     = getgid();
    
    if(dir->attributes & ATTR_DIRECTORY || dir->attributes == ATTR_VOLUME_ID)
        st->st_mode = S_IFDIR | 0755;
    if(dir->attributes & ATTR_ARCHIVE)
        st->st_mode = S_IFREG | 0644;
    
    st->st_blksize = get_cluster_size();
    
    return 0;
}

s32 lookup_short_entry(const char *path, u32 *starting_cluster, Directory *dir)
{
    char *new_path      = NULL;
    char pretty_name[13];
    u32 current_cluster = get_root_cluster();
    u32 offset          = 0;
    u32 ret             = -1;
    char *token         = NULL;
    char *intbuff       = NULL;
    Directory directory;
    
    new_path = malloc(strlen(path + 1) + 1);
    memset(new_path, 0, strlen(path + 1) + 1);
    memcpy(new_path, path + 1, strlen(path + 1));
    memset(&directory, 0, sizeof(Directory));
    
    token = strtok_r(new_path, "/", &intbuff);
    
    if(token == NULL)
        ret = 0;
    
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
        break;
    }
    
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

