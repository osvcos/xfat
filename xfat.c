#include <ctype.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "datetime.h"
#include "directory.h"
#include "reserved.h"
#include "xfat.h"

static s32 fd;

static u64 fat_region_offset;
static u16 bytes_per_sector;
static u64 data_region_offset;
static u32 cluster_size;
static u32 root_cluster;
static u32 backup_boot_sector_cluster;
static u32 cluster_count;
static u32 free_clusters;
static u32 next_free_cluster;
static u8  is_mirroring_enabled;
static u8  number_of_fats;
static u32 fat_size_32;
static u32 fsi_sector;

void close_device()
{
	close(fd);
}

s32  find_free_fat_entry(u32 *free_entry)
{
    u64 fat_offset = fat_region_offset + 8;
    u64 current_offset = fat_offset;
    u32 current_entry = 0;
    
    do
    {
        if(pread(fd, &current_entry, 4, current_offset) == -1)
            return -1;
        
        current_offset += 4;
    }
    while(current_entry != 0);
    
    *free_entry = ((current_offset - fat_offset) / 4) + 1;
    
    return 0;
}

u32 get_cluster_size()
{
    return cluster_size;
}

u32 get_data_clusters_count()
{
    return cluster_count;
}

u32 get_free_clusters_count()
{
    return free_clusters;
}

s32 get_next_entry(u32 fat_index, fat_entry *fe)
{
    u64 offset = fat_region_offset + (4 * fat_index);
    fe->current_entry = fat_index;
    fe->data_offset = 0;

    if(pread(fd, &fe->next_entry, 4, offset) == -1)
        return -1;
    
    if(fat_index >= 2)
    {
        fe->data_offset = (data_region_offset
            + ((u64) cluster_size * ((u64) fat_index - 2)));
    }
    
	return 0;
}

u32 get_root_cluster()
{
    return root_cluster;
}

s32 open_device(const char* dev)
{
    CBPB common_bpb;
    FAT32BPB fat32_bpb;
    FSINFO fsi;
    u16 fat_in_use = 0;
    u32 data_sectors = 0;

    memset(&common_bpb, 0, sizeof(CBPB));
    memset(&fat32_bpb, 0, sizeof(FAT32BPB));
    memset(&fsi, 0, sizeof(FSINFO));

    fd = open(dev, O_RDWR | O_SYNC | O_RSYNC);

    if(fd < 0)
        return -1;

    if(pread(fd, &common_bpb, sizeof(CBPB), 0) == -1)
        return -1;

    if(pread(fd, &fat32_bpb, sizeof(FAT32BPB), sizeof(CBPB)) == -1)
        return -1;

    if(pread(fd, &fsi, sizeof(FSINFO), sizeof(CBPB)
            + sizeof(FAT32BPB)) == -1)
        return -1;
    
    bytes_per_sector = common_bpb.bytes_per_sector;
    
    if(fat32_bpb.ext_flags & 0x0080)
    {
        is_mirroring_enabled = 0;
        printf("open_device: mirroring is disabled\n");
        fat_in_use = (fat32_bpb.ext_flags & 0x000F);
        printf("open_device: using fat %u\n", fat_in_use);
    }
    else
    {
        is_mirroring_enabled = 1;
        printf("open_device: mirroring is enabled\n");
    }

    fat_region_offset = (common_bpb.reserved_sectors * bytes_per_sector)
        + (fat_in_use * fat32_bpb.fat_size_32 * bytes_per_sector);

    data_region_offset = (common_bpb.reserved_sectors * bytes_per_sector)
        + ((fat32_bpb.fat_size_32 * bytes_per_sector) * common_bpb.fat_count);

    fat_size_32 = fat32_bpb.fat_size_32;
    
    number_of_fats = common_bpb.fat_count;
    
    data_sectors = common_bpb.total_sectors_32 - common_bpb.reserved_sectors
        - (fat32_bpb.fat_size_32 * common_bpb.fat_count);
    
    cluster_size = bytes_per_sector * common_bpb.sectors_per_cluster;
    root_cluster = fat32_bpb.root_cluster;
    backup_boot_sector_cluster = fat32_bpb.boot_sector_copy;
    fsi_sector = fat32_bpb.fs_info_cluster;
    cluster_count = data_sectors / common_bpb.sectors_per_cluster;
    
    read_fsi();

    return 0;
}

s32  read_fsi()
{
    FSINFO fsi;
    
    memset(&fsi, 0, sizeof(FSINFO));
    
    if(pread(fd, &fsi, sizeof(FSINFO), bytes_per_sector) == -1)
        return -1;
    
    free_clusters = (fsi.free_cluster_count == 0xFFFFFFFF) ? 0 : fsi.free_cluster_count;
    next_free_cluster = (fsi.first_free_cluster == 0xFFFFFFFF) ? 0 : fsi.first_free_cluster;
}

s32 read_cluster(u32 cluster_number, u64 offset, void *data, u32 size)
{
    fat_entry fe;
    u64 end_of_cluster = 0;
    
    memset(&fe, 0, sizeof(fat_entry));
    
    printf("read_cluster(cluster_number=%u, offset=%lu, size=%u\n", cluster_number, offset, size);
    
    if(get_next_entry(cluster_number, &fe) == -1)
        return -1;

    end_of_cluster = fe.data_offset + cluster_size;
    
    /*
     * Ensure that this operation does not pass
     * the cluster boundary
     */
    if((fe.data_offset + offset + size) > end_of_cluster)
    {
        printf("read_cluster: reached end of cluster\n");
        return -1;
    }
    
    /* Read and store the data into the data parameter.
     */
    if(pread(fd, data, size, fe.data_offset + offset) == -1)
        return -1;
    
    return 0;
}

s32 set_label(const char* label)
{
    u8  new_label[11];
    u32 start_cluster = get_root_cluster();
    u32 offset = 0;
    u16 current_date = 0;
    u16 current_time = 0;
    Directory dir;
    dir_info di;
    struct fat_datetime dt;
    
    memset(new_label, 0, sizeof(new_label));
    memset(&dir, 0, sizeof(Directory));
    memset(&dt, 0, sizeof(struct fat_datetime));
    memset(&di, 0, sizeof(dir_info));
    
    validate_83_name(label, strnlen(label, 11), new_label);
    
    if(fat_getdatetime(&dt) == -1)
        return -1;
    
    current_date = fat_getdate(&dt);
    current_time = fat_gettime(&dt);
    
    if(write_to_bootsector(sizeof(CBPB) + offsetof(FAT32BPB, volume_label),
        new_label, 11) == -1)
    {
        printf("set_label: could not write label to bootsector\n");
        return -1;
    }
    
    while(get_directory_entry(&start_cluster, &di, &offset) != -1)
    {
        if(di.dir.attributes == ATTR_VOLUME_ID)
        {
            if(write_to_cluster(start_cluster,
                offset - 32, new_label, 11) == -1)
            {
                printf("set_label: could not write label to directory\n");
                return -1;
            }
            
            if(write_to_cluster(start_cluster,
                offset + offsetof(Directory, last_mod_time),
                &current_time, 2) == -1)
            {
                printf("set_label: could not set last modification time\n");
                return -1;
            }
            
            if(write_to_cluster(start_cluster,
                offset + offsetof(Directory, last_mod_date),
                &current_date, 2) == -1)
            {
                printf("set_label: could not set last modification date\n");
                return -1;
            }
            
            return 0;
        }
    }
    
    dir.attributes = ATTR_VOLUME_ID;
    strncpy(dir.name, new_label, 11);
    dir.last_mod_time = current_time;
    dir.last_mod_date = current_date;
    dir.creation_time = current_time;
    dir.creation_date = current_date;
    dir.last_access_date = current_date;
    
    if(allocate_directory(get_root_cluster(), &dir) == -1)
        return -1;
    
    return 0;
}

s32  write_to_cluster(u32 cluster_number, u64 offset, void *data, u32 size)
{
    printf("write_to_cluster(cluster_number=%u, offset=%lu, size=%u)\n", cluster_number,
           offset, size);
    /*
     * TODO: Handle cluster expansion, and a lot more...
     */
    fat_entry fe;
    u64       end_of_cluster = 0;
    
    memset(&fe, 0, sizeof(fat_entry));
    
    if(get_next_entry(cluster_number, &fe) == -1)
    {
        printf("write_to_cluster: could not get next entry for cluster %u\n",
               cluster_number);
        return -1;
    }
    
    end_of_cluster = fe.data_offset + cluster_size;
    
    if((fe.data_offset + offset + size) > end_of_cluster)
    {
        printf("write_to_cluster: cannot exceed cluster boundary\n");
        return -1;
    }
    
    if(pwrite(fd, data, size, fe.data_offset + offset) == -1)
    {
        printf("write_to_cluster: failed to write to cluster\n");
        return -1;
    }
    
    return 0;
}

s32 write_to_fat_entry(u32 fat_index, u32 new_value)
{
    u32 write_times = 1;
    
    if(is_mirroring_enabled)
        write_times = number_of_fats;
    
    for(u32 i = 0; i < write_times; i++)
    {
        u64 offset = (fat_region_offset + (i * (fat_size_32 * bytes_per_sector)))
            + (4ULL * fat_index);
        
        if(pwrite(fd, &new_value, sizeof(u32), offset) == -1)
        {
            printf("write_to_fat_entry: error writing to FAT\n");
            return -1;
        }
    }
    return  0;
}

s32  write_to_fsi(u64 offset, const void* data, u32 size)
{
    u64 pri_offset = fsi_sector * bytes_per_sector;
    u64 bak_offset = 7ULL * bytes_per_sector;
    
    if(pwrite(fd, data, size, pri_offset + offset) == -1)
        return -1;
    
    if(pwrite(fd, data, size, bak_offset + offset) == -1)
        return -1;
    
    return 0;
}

s32 write_to_bootsector(u32 offset, const void* data, u32 size)
{
    u32 main_bootsector_offset = 0;
    u32 bk_bootsector_offset   = backup_boot_sector_cluster * bytes_per_sector;
    s32 retval = 0;
    
    if(pwrite(fd, data, size, main_bootsector_offset + offset) == -1)
        retval = -1;
    
    if(pwrite(fd, data, size, bk_bootsector_offset + offset) ==  -1)
        retval = -1;
    
    return retval;
}
