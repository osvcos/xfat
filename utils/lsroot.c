#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "directory.h"
#include "xfat.h"

int main(int argc, char *argv[])
{
    u32 root_cluster = 0;
    fat_entry fe;
    u32 item_count = 0;
    u32 offset = sizeof(Directory);
    Directory dir;
    u8 new_name[13];
    u8 type[4];
    
    memset(&fe, 0, sizeof(fat_entry));
    memset(&dir, 0, sizeof(Directory));
    memset(new_name, 0, 13);
    memset(type, 0, 4);

    if(argc != 2)
    {
        printf("Usage: lsroot <device>\n");
        exit(1);
    }
    
    if(open_device(argv[1]) ==  -1)
    {
        printf("Could not open the device\n");
        exit(1);
    }
    
    root_cluster = get_root_cluster();
    
    if(get_next_fat(root_cluster, &fe) == -1)
    {
        printf("Could not get next fat entry for root directory\n");
        exit(1);
    }
    
    while(1)
    {
        if(read_cluster(fe.current_entry, offset, &dir, sizeof(Directory)) == -1)
        {
            printf("lsroot: read_cluster(() returned non-zero\n");
            
            if(fe.next_entry == 0x0FFFFFF8)
            {
                printf("lsroot: this is the end of the root directory\n");
                break;
            }
            else
            {
                printf("lsroot: getting the next cluster of the root directory\n");
                if(get_next_fat(fe.next_entry, &fe) == -1)
                {
                    printf("lsroot: Could not get fat entry for next cluster\n");
                    exit(1);
                }
                
                offset = 0;
                continue;
            }
        }
        
        if(dir.name[0] == 0xE5)
        {
            printf("lsroot: skipping free entry\n");
            offset += sizeof(Directory);
            continue;
        }
        
        if(dir.name[0] == 0x00)
        {
            printf("lsroot: reached end of directory\n");
            break;
        }
        
        if(dir.attributes == ATTR_LONG_NAME)
        {
            printf("lsroot: skipping lfn entry\n");
            offset += sizeof(Directory);
            continue;
        }
        
        if(dir.attributes & ATTR_DIRECTORY)
            strncpy(type, "DIR\0", 4);
        else if(dir.attributes & ATTR_ARCHIVE)
            strncpy(type, "FIL\0", 4);
        else
            strncpy(type, "UNK\0", 4);
        
        memcpy(new_name, dir.name, 8);
        strncpy(new_name + 8, ".", 1);
        memcpy(new_name + 9, dir.name + 8, 3);

        printf("%s    %u bytes    %s\n", new_name, dir.file_size, type);
        
        offset += sizeof(Directory);
        item_count += 1;
    }
    
    close_device();
    printf("Total items: %u\n", item_count);
    
    return 0;
}
