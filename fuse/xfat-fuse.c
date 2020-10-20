#define FUSE_USE_VERSION 31

#include <errno.h>
#include <fuse.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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
    u32 starting_cluster = 0;
    u32 off = 0;
    Directory dir;
    u8 pretty_name[13];
    struct stat st;
    
    memset(&dir, 0, sizeof(Directory));
    memset(pretty_name, 0, 13);
    memset(&st, 0, sizeof(struct stat));
    
    printf("xfat_readdir(path=%s, offset=%u\n", path, offset);
    
    if(lookup_short_entry(path, &starting_cluster, NULL) == -1)
    {
        printf("xfat_readdir: not found\n");
        return -ENOENT;
    }
    
    if(starting_cluster != get_root_cluster())
    {
        off = sizeof(Directory) * 2;
        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);
    }
    
    while(get_directory_entry(&starting_cluster, &dir, &off) != -1)
    {
        if(dir.attributes == ATTR_VOLUME_ID)
            continue;
        if(dir.name[0] == 0xE5)
            continue;
        if(dir.attributes == ATTR_LONG_NAME)
            continue;
        
        prettify_83_name(dir.name, pretty_name);
        get_stat_from_directory(&dir, &st);
        filler(buf, pretty_name, &st, 0);
    }
    
    return 0;
}

static int xfat_getattr(const char *path, struct stat *stbuf)
{
    u8 pretty_name[13];
    u32 starting_cluster = 0;
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
            return 0;
        }
        else
        {
            printf("xfat_getattr: first entry in root directory is not volume label\n");
            
            stbuf->st_size = 0;
            stbuf->st_mode = S_IFDIR | 0755;
            stbuf->st_uid     = getuid();
            stbuf->st_gid     = getgid();
            return 0;
        }
        
        stbuf->st_nlink = 2;
    }
    
    if(lookup_short_entry(path, &starting_cluster, &dir) == -1)
    {
        printf("xfat_getattr: %s not found\n", path);
        return -ENOENT;
    }
    
    printf("xfat_getattr: found match for %s\n", pretty_name);
    get_stat_from_directory(&dir, stbuf);
    stbuf->st_nlink = 1;
    
    return 0;
}

static int xfat_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
    Directory dir;
    u32 off = 0;
    u32 starting_cluster = 0;
    u32 root = get_root_cluster();
    u8 pretty_name[13];
    
    memset(&dir, 0, sizeof(Directory));
    memset(pretty_name, 0, 13);
    
    printf("xfat_read(path=%s, size=%lu, offset=%lu)\n", path, size, offset);
    
    if(lookup_short_entry(path, &starting_cluster, &dir) == -1)
        return -ENOENT;
    
    prettify_83_name(dir.name, pretty_name);
        
    u32 current_cluster = (dir.first_clus_hi << 16) | dir.first_clus_low;
    u32 prev_cluster    = 0;
    s32 change_times      = 0;
    u32 offset_left       = 0;
    s64 current_size      = size;
    u64 read_in_cluster   = 0;
    u64 bytes_read        = 0;
    
    printf("xfat_read: found %s, start cluster is %u\n", pretty_name, current_cluster);
    
    if(offset >= get_cluster_size())
    {
        change_times = offset / get_cluster_size();
        offset_left  = offset % get_cluster_size();
    }
    else
    {
        offset_left = offset;
    }
    
    printf("xfat_read: change_times=%d, offset_left=%d\n", change_times, offset_left);
    
    while(change_times-- > 0)
    {
        fat_entry fe;
        
        prev_cluster = current_cluster;
        
        if(get_next_fat(current_cluster, &fe) == -1)
        {
            printf("xfat_read: could not change to next cluster\n");
            return 0;
        }
        
        current_cluster = fe.next_entry;
        
        printf("xfat_read: changed to cluster %d\n", current_cluster);
    }
    
    while(current_size >= 1)
    {
        printf("xfat_read: current_size = %d\n", current_size);
        
        u32 bytes_to_read = 0;
        
        if((current_size + offset_left) >= get_cluster_size())
            bytes_to_read = get_cluster_size() - offset_left;
        else
            bytes_to_read = current_size;
        
        current_size -= bytes_to_read;
        
        if(read_in_cluster >= get_cluster_size())
        {
            printf("xfat_read: reached cluster limit, changing cluster\n");
            printf("xfat_read: read_in_cluster = %u\n", read_in_cluster);
            printf("xfat_read: current_cluster = %u\n", current_cluster);
            
            fat_entry fe;
            
            if(get_next_fat(current_cluster, &fe) == -1)
            {
                printf("xfat_read: error while changing fat entry\n");
                return 0;
            }
            
            current_cluster = fe.next_entry;
            read_in_cluster = 0;
            
            printf("xfat_read: changed to cluster %u\n", current_cluster);
        }
        
        if(current_cluster == 0xFFFFFFFF)
        {
            printf("xfat_read: reached end of chain\n");
            return bytes_read;
        }
        
        if(read_cluster(current_cluster, offset_left, buf + bytes_read, bytes_to_read) == -1)
        {
            printf("xfat_read: could not read data\n");
            return 0;
        }
        
        printf("xfat_read: read operation succeded\n");
        
        bytes_read += bytes_to_read;
        read_in_cluster = get_cluster_size();
        offset_left = 0;
    }
    
    return bytes_read;
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
