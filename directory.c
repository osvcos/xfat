#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "datetime.h"
#include "directory.h"
#include "log.h"
#include "superblock.h"
#include "xfat.h"

typedef struct {
    u8 name1[5];
    u8 name2[6];
    u8 name3[2];
} names;

static u32 is_forbidden_char(u8 c)
{
    u8 forbidden_chars[] = {
        0x22, 0x2a, 0x2b, 0x2c,
        0x2e, 0x2f, 0x3a, 0x3b,
        0x3c, 0x3d, 0x3e, 0x3f,
        0x5b, 0x5c, 0x5d, 0x7c
    };
    u32 low  = 0;
    u32 high = sizeof(forbidden_chars) - 1;
    u32 mid  = 0;

    while(low <= high)
    {
        mid = (low + ((high - low) / 2));

        if(forbidden_chars[mid] > c)
        {
            high = mid - 1;
            continue;
        }
        if(forbidden_chars[mid] < c)
        {
            low = mid + 1;
            continue;
        }
        if(forbidden_chars[mid] == c)
            return 1;
    }

    return 0;
}

static void prettify_83_name(u8 *restrict input_name,
                             u8 *restrict output_name)
{
    u32 name_len = 0;
    
    for(name_len = 0; name_len < 8; name_len++)
    {
        if(input_name[name_len] == 0x20)
            break;
        output_name[name_len] = input_name[name_len];
    }
    
    if(input_name[8] != 0x20
        || input_name[9] != 0x20
        || input_name[10] != 0x20)
    {
        strncat(output_name, ".\0", 2);
        
        for(int i = 0; i < 3; i++)
        {
            u8 c = input_name[i + 8];
            
            if(c == 0x20)
                break;
            
            memcpy(output_name + (i + name_len + 1), &c, 1);
        }
    }
}

static void to_utf8(u8 *restrict input, u32 osize,
                    u8 *restrict output)
{
    for(int i = 0; i < osize; i ++)
    {
        u8 *c = ((u8*) (u16*) input) + (i * 2);
        output[i] = (*c == 0xFF) ? 0x00 : *c;
    }
}

s32 allocate_directory(u32 directory_cluster, Directory *dir)
{
    Directory temp_dir;
    Directory this_directory;
    Directory parent_directory;
    u32 offset = 0;
    u32 parent_directory_cluster = directory_cluster;
    u32 this_directory_cluster = 0;
    fat_entry next;
    struct fat_datetime datetime;
    
    memset(&temp_dir, 0, sizeof(Directory));
    memset(&this_directory, 0, sizeof(Directory));
    memset(&parent_directory, 0, sizeof(Directory));
    memset(&next, 0, sizeof(fat_entry));
    memset(&datetime, 0, sizeof(struct fat_datetime));
    
    do
    {
        if(read_cluster(parent_directory_cluster, offset, 
            &temp_dir, sizeof(Directory)) == -1)
        {
            if(get_next_entry(parent_directory_cluster, &next) == -1)
            {
                LOG("allocate_directory: (1) could not get next entry\n");
                return -1;
            }
            
            if(next.next_entry == 0x0FFFFFF8)
            {
                u32 next_entry = 0;
                
                if(find_free_fat_entry(&next_entry) == -1)
                {
                    LOG("allocate_directory: (1) could not get next free cluster\n");
                    return -1;
                }
                
                if(write_to_fat_entry(parent_directory_cluster, next_entry) == -1)
                {
                    LOG("allocate_directory: (1) could not write to fat entry %u\n",
                            parent_directory_cluster);
                    return -1;
                }

                if(write_to_fat_entry(next_entry, 0xFFFFFF8) == -1)
                {
                    LOG("allocate_directory: (2) could not write to fat entry %u\n", next_entry);
                    return -1;
                }
                
                offset = 0;
                continue;
            }
            else
            {
                parent_directory_cluster = next.next_entry;
                offset = 0;
                continue;
            }
        }
        
        if(temp_dir.name[0] == 0x00
            || temp_dir.name[0] == 0xE5)
        {
            memcpy(&temp_dir, dir, sizeof(Directory));
            
            if(temp_dir.attributes != ATTR_VOLUME_ID)
            {
                if(find_free_fat_entry(&this_directory_cluster) == -1)
                {
                    LOG("allocate_directory: (2) could not get next free cluster\n");
                    return -1;
                }

                LOG("allocate_directory: free cluster entry: %u\n", this_directory_cluster);
                
                if(write_to_fat_entry(this_directory_cluster, 0x0FFFFFFF) == -1)
                    return -1;
                
                temp_dir.first_clus_hi  = (this_directory_cluster & 0xFFFF0000) >> 16;
                temp_dir.first_clus_low = (this_directory_cluster & 0x0000FFFF);
                
                LOG("allocate_directory: clus_hi: %u, clus_lo: %u\n", 
                       temp_dir.first_clus_hi, temp_dir.first_clus_low);
                
                u32 current_free_clusters_count = get_free_clusters_count();
                u32 new_free_clusters = 0;
                u32 new_free_entry = 0;
                
                if(current_free_clusters_count != 0)
                    new_free_clusters = current_free_clusters_count - 1;
                
                if(write_to_fsi(offsetof(FSINFO, free_cluster_count),
                        &new_free_clusters, 4) == -1)
                {
                    LOG("allocate_directory: could not set the new free cluster count\n");
                    return  -1;
                }
                
                if(find_free_fat_entry(&new_free_entry) == -1)
                {
                    LOG("allocate_directory: could not get a new free entry\n");
                    return -1;
                }
                
                if(write_to_fsi(offsetof(FSINFO, first_free_cluster), &new_free_entry, 4) == -1)
                {
                    LOG("allocate_directory: could not set the new free entry\n");
                    return -1;
                }
                
                read_fsi();
            }
            
            if(fat_getdatetime(&datetime) == -1)
            {
                LOG("allocate_directory: could not get current datetime\n");
                return -1;
            }
            
            temp_dir.creation_time    = fat_gettime(&datetime);
            temp_dir.creation_date    = fat_getdate(&datetime);
            temp_dir.last_access_date = fat_getdate(&datetime);
            temp_dir.last_mod_time    = fat_gettime(&datetime);
            temp_dir.last_mod_date    = fat_getdate(&datetime);
            
            if(write_to_cluster(parent_directory_cluster, offset, &temp_dir, sizeof(Directory)) == -1)
            {
                LOG("allocate_directory: could not write to cluster\n");
                return -1;
            }
           
           if(temp_dir.attributes & ATTR_DIRECTORY)
           {
                strncpy(this_directory.name, ".          ", 11);
                strncpy(parent_directory.name, "..         ", 11);
                
                this_directory.attributes = ATTR_DIRECTORY;
                parent_directory.attributes = ATTR_DIRECTORY;
                
                this_directory.first_clus_hi = (this_directory_cluster & 0xFFFF0000) >> 16;
                this_directory.first_clus_low = (this_directory_cluster & 0x0000FFFF);
                
                if(directory_cluster == get_root_cluster())
                {
                    parent_directory.first_clus_hi = 0;
                    parent_directory.first_clus_low = 0;
                }
                else
                {
                    parent_directory.first_clus_hi = (parent_directory_cluster & 0xFFFF0000) >> 16;
                    parent_directory.first_clus_low = (parent_directory_cluster & 0x0000FFFF);
                }
                
                /*
                * TODO: Timestamps?
                */
                
                if(write_to_cluster(this_directory_cluster, 0, &this_directory, 
                    sizeof(Directory)) == -1)
                {
                    LOG("allocate_directory: could not create . entry\n");
                    return -1;
                }
                
                if(write_to_cluster(this_directory_cluster, sizeof(Directory), &parent_directory,
                    sizeof(Directory)) == -1)
                {
                    LOG("allocate_directory: could not create .. entry\n");
                    return -1;
                }
           }
           
           return 0;
        }
        
        offset += sizeof(Directory);
    }
    while(1);
    
    return 0;
}

s32 get_directory_entry(u32 *cluster_number, dir_info *di, u32 *offset)
{
    fat_entry fe;
    lfn_entry lfn;
    names *lfns = NULL;
    u32 is_first_lfn = 1;
    u32 lfn_entries = 0;
    
    memset(di->long_name, 0, MAX_LFN_LENGTH);
    memset(&fe, 0, sizeof(fat_entry));
    memset(&lfn, 0, sizeof(lfn_entry));

    /*
     * Check if offset is aligned to size
     * of Directory structure
     */
    if((*offset % sizeof(Directory)) != 0)
    {
        LOG("get_directory_entry: offset not aligned\n");
        return -1;
    }

next_cluster:
    if(get_next_entry(*cluster_number, &fe) == -1)
    {
        LOG("get_directory_entry: could not get next fat entry\n");
        return -1;
    }

read_cluster:
    if(read_cluster(fe.current_entry, *offset, &di->dir, sizeof(Directory)) == -1)
    {
        if(fe.next_entry == 0x0FFFFFF8)
        {
            LOG("get_directory_entry: reached end of directory\n");
            return -1;
        }

        *cluster_number = fe.next_entry;
        *offset = 0;
        goto next_cluster;
    }

    /*
     * The previous entry was the last allocated
     * entry in the directory
     */
    if(di->dir.name[0] == 0x00)
    {
        LOG("get_directory_entry: end of directory\n");
        *offset = 0;
        return -1;
    }

    if(di->dir.attributes == ATTR_LONG_NAME)
    {
        memcpy(&lfn, &di->dir, sizeof(lfn_entry));
        
        if(is_first_lfn)
        {
            lfn_entries = (lfn.ordinal - 0x40) - 1;
            
            LOG("get_directory_entry: lfn entries: %u\n", lfn_entries);
            is_first_lfn = 0;
        }
        
        lfns = malloc(sizeof(names));
        memset(lfns, 0, sizeof(names));
        
        to_utf8(lfn.name1, 5, lfns->name1);
        to_utf8(lfn.name2, 6, lfns->name2);
        to_utf8(lfn.name3, 2, lfns->name3);

        memcpy(di->long_name + (sizeof(names) * lfn_entries--),
               lfns, sizeof(names));
        *offset += sizeof(Directory);
        free(lfns);
        goto read_cluster;
    }
    else
    {
        if(lfn_entries == 0)
            prettify_83_name(di->dir.name, di->long_name);
    }
    
    LOG("get_directory_entry: final lfn %s\n", di->long_name);

    di->cluster32 = (di->dir.first_clus_hi << 16)
        | di->dir.first_clus_low;

    *offset += sizeof(Directory);
    return 0;
}

void validate_83_name(const u8 *input_name, u32 input_name_size, u8 *output_name)
{
    u8  new_name[11];

    memset(new_name, 0x20, 11);

    for(s32 i = 0; i < input_name_size; i++)
    {
        if(is_forbidden_char(input_name[i]))
        {
            new_name[i] = 0x20;
            continue;
        }
        
        new_name[i] = (islower(input_name[i])) ? toupper(input_name[i])
            : input_name[i];
    }

    memcpy(output_name, new_name, 11);
}
