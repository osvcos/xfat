#include <stdio.h>
#include <string.h>

#include "directory.h"
#include "xfat.h"

void usage();

void usage()
{
    printf("Creates a directory in the root directory of the specified device\n\n");
    printf("Usage: mkdir <device> <directory-name>\n");
}

int main(int argc, char *argv[])
{
    char new_name[11];
    dir_info di;
    u32  root_cluster = 0;
    u32 offset = 0;
    Directory dir;
    
    memset(new_name, 0, sizeof(new_name));
    memset(&di, 0, sizeof(dir_info));
    memset(&dir, 0, sizeof(Directory));
    
    if(argc != 3)
    {
        usage();
        return 1;
    }
    
    if(open_device(argv[1]) == -1)
    {
        printf("Could not open device\n");
        return 1;
    }
    
    root_cluster = get_root_cluster();
    
    validate_83_name(argv[2], strnlen(argv[2], 11), new_name);
    
    printf("root cluster = %d\n", get_root_cluster());
    
    while(get_directory_entry(&root_cluster, &di, &offset) != -1)
    {
        if(di.dir.attributes == ATTR_VOLUME_ID)
            continue;
        if(di.dir.attributes == ATTR_LONG_NAME)
            continue;
        
        if(strncmp(new_name, di.dir.name, 11) == 0)
        {
            printf("Directory already exists on the filesystem\n");
            return 1;
        }
    }
    
    memcpy(dir.name, new_name, sizeof(new_name));
    dir.attributes = ATTR_DIRECTORY;
    
    if(allocate_directory(root_cluster, &dir) == -1)
    {
        printf("Could not create directory\n");
        return 1;
    }
    
    close_device();
    
    return 0;
}
