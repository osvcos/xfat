#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "directory.h"
#include "xfat.h"

typedef struct {
    u8 name1[6];
    u8 name2[7];
    u8 name3[3];
} names;

static s32 prettify_83_name(u8 *input_name, u8 *output_name)
{
    memcpy(output_name, input_name, 8);
    
    if(input_name[8] != 0x20
        || input_name[9] != 0x20
        || input_name[10] != 0x20)
    {
        strncpy(output_name + 8, ".", 1);
        memcpy(output_name + 9, input_name + 8, 3);
    }
}

static void to_utf8(u8 *input, u32 osize, u8 *output) {
    u32 index = 0;
    u32 step = 0;
    
    for(int i = 0; i < osize; i ++)
    {
        u8 *c = ((u8*) (u16*) input) + step;
        
        if(*c == 0xFF)
            *c = 0x0;
        
        output[index++] = *c;
        step += 2;
    }
}

s32 get_directory_entry(u32 *cluster_number, dir_info *di, u32 *offset)
{
    fat_entry fe;
    lfn_entry lfn;
    names *lfns = NULL;
    u32 lfn_count = 0;
    
    lfns = malloc(0);
    memset(di->long_name, 0, MAX_LFN_LENGTH);
    memset(&fe, 0, sizeof(fat_entry));
    memset(&lfn, 0, sizeof(lfn_entry));

    /*
     * Check if offset is aligned to size
     * of Directory structure
     */
    if((*offset % sizeof(Directory)) != 0)
    {
        printf("get_directory_entry: position not aligned\n");
        return -1;
    }

next_cluster:
    if(get_next_entry(*cluster_number, &fe) == -1)
    {
        printf("get_directory_entry: could not get next fat entry\n");
        return -1;
    }

read_cluster:
    if(read_cluster(fe.current_entry, *offset, &di->dir, sizeof(Directory)) == -1)
    {
        if(fe.next_entry == 0x0FFFFFF8)
        {
            printf("get_directory_entry: reached end of directory\n");
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
        printf("get_directory_entry: end of directory\n");

        *offset = 0;
        return -1;
    }

    if(di->dir.attributes == ATTR_LONG_NAME)
    {
        memcpy(&lfn, &di->dir, sizeof(lfn_entry));
        
        lfns = realloc(lfns, (sizeof(names) * (lfn_count + 1)));
        
        memset(lfns[lfn_count].name1, 0, 6);
        memset(lfns[lfn_count].name2, 0, 7);
        memset(lfns[lfn_count].name3, 0, 3);
        
        to_utf8(lfn.name1, 5, lfns[lfn_count].name1);
        to_utf8(lfn.name2, 6, lfns[lfn_count].name2);
        to_utf8(lfn.name3, 2, lfns[lfn_count].name3);

        *offset += sizeof(Directory);
        lfn_count += 1;
        goto read_cluster;
    }
    
    if(lfn_count != 0)
    {
        for(int i = lfn_count - 1; i >= 0; i--)
        {
            strncat(di->long_name, lfns[i].name1, 5);
            strncat(di->long_name, lfns[i].name2, 7);
            strncat(di->long_name, lfns[i].name3, 3);
        }
    }
    else
        prettify_83_name(di->dir.name, di->long_name);
    
    printf("get_directory_entry: final lfn %s\n", di->long_name);
    
    *offset += sizeof(Directory);
    memset(lfns, 0, (sizeof(names) * lfn_count));
    free(lfns);
    return 0;
}
