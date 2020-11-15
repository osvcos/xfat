#define FUSE_USE_VERSION 31

#include <errno.h>
#include <fuse.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/statvfs.h>
#include <unistd.h>

#include "directory.h"
#include "utils.h"
#include "xfat.h"

static void usage()
{
    printf("Usage: xfat-fuse <device> <mountpoint>\n");
}

static void xfat_destroy(void *data)
{
    close_device();
}

static int xfat_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi)
{
    u32 starting_cluster = 0;
    u32 off = 0;
    dir_info di;
    struct stat st;
    
    memset(&di, 0, sizeof(dir_info));
    memset(&st, 0, sizeof(struct stat));
    
    printf("xfat_readdir(path=%s, offset=%u\n", path, offset);
    
    if(lookup_entry(path, &starting_cluster, NULL) == -1)
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
    
    while(get_directory_entry(&starting_cluster, &di, &off) != -1)
    {
        if(di.dir.attributes == ATTR_VOLUME_ID)
            continue;
        if(di.dir.name[0] == 0xE5)
            continue;
        
        get_stat_from_directory(&di.dir, &st);
        filler(buf, di.long_name, &st, 0);
    }
    
    return 0;
}

static int xfat_getattr(const char *path, struct stat *stbuf)
{
    u32 starting_cluster = get_root_cluster();
    dir_info di;
    u32 offset = 0;
    
    memset(&di, 0, sizeof(dir_info));
    
    printf("xfat_getattr(path=%s)\n", path);
    
    if(strncmp(path, "/\0", 2) == 0)
    {
        get_directory_entry(&starting_cluster, &di, &offset);
        
        if(di.dir.attributes == ATTR_VOLUME_ID)
        {
            get_stat_from_directory(&di.dir, stbuf);
        }
        else
        {
            stbuf->st_size = 0;
            stbuf->st_mode = S_IFDIR | 0755;
            stbuf->st_uid     = getuid();
            stbuf->st_gid     = getgid();
        }
        
        stbuf->st_nlink = 2;
        return 0;
    }
    
    if(lookup_entry(path, &starting_cluster, &di) == -1)
    {
        printf("xfat_getattr: %s not found\n", path);
        return -ENOENT;
    }
    
    printf("xfat_getattr: found match for %s\n", di.long_name);
    get_stat_from_directory(&di.dir, stbuf);
    stbuf->st_nlink = 1;
    
    return 0;
}

static int xfat_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
    dir_info di;
    u32 starting_cluster = 0;
    u32 current_cluster = 0;
    s32 change_times      = 0;
    u32 offset_left       = 0;
    s64 current_size      = size;
    u64 bytes_read        = 0;
    u32 bytes_to_read = 0;
    fat_entry next;
    
    memset(&di, 0, sizeof(dir_info));
    memset(&next, 0, sizeof(fat_entry));
    
    printf("xfat_read(path=%s, size=%lu, offset=%lu)\n", path, size, offset);
    
    if(lookup_entry(path, &starting_cluster, &di) == -1)
        return -ENOENT;
        
    current_cluster = get_cluster32(di.dir.first_clus_hi, di.dir.first_clus_low);
    
    printf("xfat_read: found %s, start cluster is %u\n", di.long_name, current_cluster);
    
    if(offset >= get_cluster_size())
    {
        change_times = offset / get_cluster_size();
        offset_left  = offset % get_cluster_size();
    }
    else
        offset_left = offset;
    
    printf("xfat_read: change_times=%d, offset_left=%d\n", change_times, offset_left);
    
    while(change_times-- > 0)
    {
        if(get_next_entry(current_cluster, &next) == -1)
        {
            printf("xfat_read: could not change to next cluster\n");
            return 0;
        }
        
        current_cluster = next.next_entry;
        
        printf("xfat_read: changed to cluster %d\n", current_cluster);
    }
    
    while(current_size >= 1)
    {
        printf("xfat_read: current_size = %d\n", current_size);

        if((current_size + offset_left) >= get_cluster_size())
            bytes_to_read = get_cluster_size() - offset_left;
        else
            bytes_to_read = current_size;

        current_size -= bytes_to_read;

        if(read_cluster(current_cluster, offset_left, buf + bytes_read, bytes_to_read) == -1)
        {
            printf("xfat_read: could not read data\n");
            return 0;
        }

        printf("xfat_read: changing cluster\n");
        printf("xfat_read: current_cluster = %u\n", current_cluster);

        if(get_next_entry(current_cluster, &next) == -1)
        {
            printf("xfat_read: error while changing fat entry\n");
            return 0;
        }
        
        current_cluster = next.next_entry;

        printf("xfat_read: changed to cluster %u\n", current_cluster);

        bytes_read += bytes_to_read;
        offset_left = 0;
        bytes_to_read = 0;
    }
    
    return bytes_read;
}

static int xfat_open(const char *path, struct fuse_file_info *fi)
{
    printf("xfat_open(path=%s)\n", path);
    return 0;
}

static int xfat_statfs(const char *path, struct statvfs *stvfs)
{
    memset(stvfs, 0, sizeof(struct statvfs));
    
    printf("xfat_statfs(path=%s)\n", path);
    
    stvfs->f_bsize  = get_cluster_size();
    stvfs->f_frsize = get_cluster_size();
    stvfs->f_blocks = get_data_cluster_count();
    stvfs->f_bfree  = get_free_clusters_count();
    stvfs->f_bavail = get_free_clusters_count();
    
    return 0;
}

static struct fuse_operations xfat_ops = {
    .destroy  = xfat_destroy,
    .readdir  = xfat_readdir,
    .getattr  = xfat_getattr,
    .read     = xfat_read,
    .open     = xfat_open,
    .statfs   = xfat_statfs
};

int main(int argc, char *argv[])
{
    char noptions[500];
    char dev_realpath[300];
#ifndef DEBUG
    int nargc = 4;
    char *nargv[4];
#else
    int nargc = 6;
    char *nargv[6];
#endif
    
    char *option = "-o";
    char *options = "allow_other,fsname=";
#ifdef DEBUG
    char *opt_debug = "-o";
    char *debug = "-f";
#endif
    
    memset(noptions, 0, sizeof(noptions));
    memset(dev_realpath, 0, sizeof(dev_realpath));
    strncpy(noptions, options, sizeof(noptions));
    
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
#ifdef DEBUG
    nargv[3] = opt_debug;
    nargv[4] = debug;
    nargv[5] = argv[2];
#else
    nargv[3] = argv[2];
#endif
    realpath(argv[1], dev_realpath);
    strncat(noptions, dev_realpath, sizeof(dev_realpath));
    nargv[2] = noptions;
    
    return fuse_main(nargc, nargv, &xfat_ops, NULL);
}
