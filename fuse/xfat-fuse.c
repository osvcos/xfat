#define FUSE_USE_VERSION 31

#include <errno.h>
#include <fuse.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "directory.h"
#include "utils.h"
#include "xfat.h"

static void usage()
{
    
}

static void xfat_destroy(void *data)
{
    close_device();
}

static int xfat_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi)
{
    u32 root = get_root_cluster();
    u32 off = 0;
    Directory dir;
    u8 pretty_name[13];
    
    memset(&dir, 0, sizeof(Directory));
    memset(pretty_name, 0, 13);
    
    printf("xfat_readdir(path=%s, offset=%u\n", path, offset);
    
    if(strncmp(path, "/", 1) != 0)
        return -ENOENT;
    
    while(get_directory_entry(&root, &dir, &off) != -1)
    {
        if(dir.attributes == ATTR_VOLUME_ID)
            continue;
        if(dir.name[0] == 0xE5)
            continue;
        if(dir.attributes == ATTR_LONG_NAME)
            continue;
        
        prettify_83_name(dir.name, pretty_name);
        filler(buf, pretty_name, NULL, 0);
    }
    
    return 0;
}

static int xfat_getattr(const char *path, struct stat *stbuf)
{
    u8 pretty_name[13];
    Directory dir;
    u32 offset = 0;
    u32 root = get_root_cluster();
    
    memset(pretty_name, 0, 13);
    memset(&dir, 0, sizeof(Directory));
    
    printf("xfat_getattr(path=%s)\n", path);
    
    if(strncmp(path, "/\0", 2) == 0)
    {
        get_directory_entry(&root, &dir, &offset);
        
        if(dir.attributes == ATTR_VOLUME_ID)
        {
            get_stat_from_directory(&dir, stbuf);
            stbuf->st_nlink = 2;
            return 0;
        }
        else
        {
            printf("xfat_getattr: first entry in root directory is not volume label\n");
            return 0;
        }
    }
    
    while(get_directory_entry(&root, &dir, &offset) != -1)
    {
        prettify_83_name(dir.name, pretty_name);
        
        if(strncmp(path + 1, pretty_name, 13) == 0)
        {
            printf("xfat_getattr: found match for %s\n", pretty_name);
            get_stat_from_directory(&dir, stbuf);
            stbuf->st_nlink = 1;
            break;
        }
    }
    
    return 0;
}

static int xfat_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
    printf("xfat_read(path=%s, size=%lu, offset=%lu)\n", path, size, offset);
    return 0;
}

static int xfat_open(const char *path, struct fuse_file_info *fi)
{
    printf("xfat_open(path=%s)\n", path);
    return 0;
}

static struct fuse_operations xfat_ops = {
    .destroy  = xfat_destroy,
    .readdir  = xfat_readdir,
    .getattr  = xfat_getattr,
    .read     = xfat_read,
    .open     = xfat_open,
};

int main(int argc, char *argv[])
{
#ifndef DEBUG
    int nargc = 4;
    char *nargv[4];
#else
    int nargc = 6;
    char *nargv[6];
#endif
    
    char *option = "-o";
    char *options = "allow_other";
#ifdef DEBUG
    char *opt_debug = "-o";
    char *debug = "-f";
#endif
    
    if(argc != 3)
    {
        usage();
        return 1;
    }
    
    if(open_device(argv[1]) == -1)
    {
        printf("xfat-fuse: could not open device\n");
        return 1;
    }
    
    nargv[0] = argv[0];
    nargv[1] = option;
    nargv[2] = options;
#ifdef DEBUG
    nargv[3] = opt_debug;
    nargv[4] = debug;
    nargv[5] = argv[2];
#else
    nargv[3] = argv[2];
#endif
    
    return fuse_main(nargc, nargv, &xfat_ops, NULL);
}
