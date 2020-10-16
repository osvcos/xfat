#include <string.h>
#include <unistd.h>

#include "utils.h"
#include "xfat.h"

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
