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
    u32 offset = 0;
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
    
    while(get_directory_entry(&root_cluster, &dir, &offset) != -1)
    {
        if(dir.name[0] == 0xE5)
            continue;
        
        if(dir.attributes != ATTR_LONG_NAME)
            item_count += 1;
        if(dir.attributes == ATTR_LONG_NAME)
            strncpy(type, "LFN\0", 4);
        else if(dir.attributes & ATTR_ARCHIVE)
            strncpy(type, "FIL\0", 4);
        else if(dir.attributes & ATTR_DIRECTORY)
            strncpy(type, "DIR\0", 4);
        else if(dir.attributes & ATTR_VOLUME_ID)
            strncpy(type, "LBL\0", 4);
        else
            strncpy(type, "UNK\0", 4);
        
        memcpy(new_name, dir.name, 8);
        strncpy(new_name + 8, ".", 1);
        memcpy(new_name + 9, dir.name + 8, 3);
        
        printf("Entry name: %s\n", new_name);
        printf("Entry size (in bytes): %u\n", dir.file_size);
        printf("Entry type: %s\n\n", type);
    }
    
    close_device();
    printf("Total items: %u\n", item_count);
    
    return 0;
}
