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

static u32 reserved_sectors;
static u64 fat_region_offset;
static u32 bytes_per_sector;
static u64 data_region_offset;
static u32 cluster_size;
static u32 root_cluster;
static u32 backup_boot_sector_cluster;

s32 open_device(const char* dev)
{
	CBPB* cbpb = NULL;
	FAT32BPB *f32bpb = NULL;
	
	fd = open(dev, O_RDWR | O_SYNC | O_RSYNC);
	
	if(fd < 0)
		return -1;
	
	cbpb = malloc(sizeof(CBPB));
	f32bpb = malloc(sizeof(FAT32BPB));
	
	if(pread(fd, cbpb, sizeof(CBPB), 0) == -1)
		return -1;
	
	if(pread(fd, f32bpb, sizeof(FAT32BPB), sizeof(CBPB)) == -1)
		return -1;
	
	bytes_per_sector = cbpb->bytes_per_sector;
	reserved_sectors = cbpb->reserved_sectors * bytes_per_sector;
	fat_region_offset = reserved_sectors;
	data_region_offset = reserved_sectors + ((f32bpb->fat_size_32 * bytes_per_sector) * cbpb->fat_count);
	cluster_size = bytes_per_sector * cbpb->sectors_per_cluster;
	root_cluster = f32bpb->root_cluster;
	backup_boot_sector_cluster = f32bpb->boot_sector_copy;
	
	free(cbpb);
	free(f32bpb);
	
	return 0;
}

s32 get_next_fat(u32 fat_index, fat_entry *fe)
{
	fe->current_entry = fat_index;
	
	u32 next_fat;
	u32 current_fat = fat_region_offset + (4 * fat_index);
	
	if(pread(fd, &next_fat, sizeof(u32), current_fat) == -1)
		return -1;
	
	fe->next_entry = next_fat;
	
	if(fat_index == 0 || fat_index == 1)
		fe->data_offset = 0;
	else
	{
		u32 offset = data_region_offset + (cluster_size * (fat_index - 2));
		fe->data_offset = offset;
	}
	
	return 0;
}

u32 get_root_cluster()
{
	return root_cluster;
}

s32 read_cluster(u64 start_of_cluster, u32 offset, void *data, u32 size)
{
    printf("start_of_cluster=%lu, offset=%u, size=%u\n", start_of_cluster, offset, size);
    
    u64 end_of_cluster = (start_of_cluster + cluster_size);
    u64 cluster_offset = start_of_cluster + offset;
    
    printf("end_of_cluster=%lu\n", end_of_cluster);
    
    /* Check if the starting cluster offset
     * is aligned to the cluster size
     */
    if((end_of_cluster % cluster_size) != 0)
    {
        printf("read_cluster: Start of cluster is not aligned\n");
        return -1;
    }
    
    /*
     * Ensure that this operation does not pass
     * the cluster boundary
     */
    if((start_of_cluster + offset + size) >= end_of_cluster)
    {
        printf("read_cluster: reached end of cluster\n");
        return -1;
    }
    
    /* Read and store the data in the data parameter.
     */
    if(pread(fd, data, size, cluster_offset) == -1)
        return -1;
    
    return 0;
}

s32 set_label(const char* label)
{
	fat_entry fe;
    u32 label_offset = 0;
    Directory dir;
    struct fat_datetime dt;
	
    memset(&dir, 0, sizeof(Directory));
    
	if(strlen(label) < 11)
		return -1;
	
    if(get_next_fat(root_cluster, &fe) == -1)
        return -1;
    
    if(pread(fd, &dir, sizeof(Directory), fe.data_offset) == -1)
        return -1;
    
    if(fat_getdatetime(&dt) == -1)
        return -1;
    
    memcpy(dir.name, label, 11);
    
    if(dir.attributes != ATTR_VOLUME_ID)
    {
        dir.attributes = ATTR_VOLUME_ID;
        dir.creation_time = fat_gettime(&dt);
        dir.creation_date = fat_getdate(&dt);
    }
    
    dir.last_access_date = fat_getdate(&dt);
    dir.last_mod_date    = fat_getdate(&dt);
    dir.last_mod_time = fat_gettime(&dt);
    
	label_offset = sizeof(CBPB) + offsetof(FAT32BPB, volume_label);
	
	if(write_to_bootsector(label_offset, label, 11) == -1)
        return -1;
	
    if(pwrite(fd, &dir, sizeof(Directory), fe.data_offset) == -1)
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

void close_device()
{
	close(fd);
}
