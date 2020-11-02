#include <stdio.h>
#include <string.h>

#include "directory.h"
#include "xfat.h"

s32 get_directory_entry(u32 *cluster_number, dir_info *di, u32 *offset)
{
    fat_entry fe;
    lfn_entry lfn;

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
        
        *offset += sizeof(Directory);
        goto read_cluster;
    }
    
    *offset += sizeof(Directory);
    return 0;
}
