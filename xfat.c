#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "reserved.h"
#include "xfat.h"

static s32 fd;

static u32 reserved_sectors;
static u32 fat_region_offset;
static u32 bytes_per_sector;
static u32 data_region_offset;
static u32 cluster_size;
static u32 root_cluster;
static u32 backup_boot_sector_cluster;

s32 open_device(const char* dev)
{
	CBPB* cbpb = NULL;
	FAT32BPB *f32bpb = NULL;
	
	fd = open(dev, O_RDWR);
	
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

s32 set_label(const char* label)
{
	fat_entry fe;
	
	if(strlen(label) < 11)
		return -1;
	
	u32 label_offset = sizeof(CBPB) + offsetof(FAT32BPB, volume_label);
	
	if(pwrite(fd, label, 11, label_offset) == -1)
		return -1;
	
	u32 backup_label_offset = (backup_boot_sector_cluster * bytes_per_sector) \
		+ sizeof(CBPB) + offsetof(FAT32BPB, volume_label);
	
	if(pwrite(fd, label, 11, backup_label_offset) == -1)
		return -1;
	
	if(get_next_fat(root_cluster, &fe) == -1)
		return -1;
	
	if(pwrite(fd, label, 11, fe.data_offset) == -1)
		return -1;
	
	u8 attrs = 0x08;
	if(pwrite(fd, &attrs, sizeof(u8), fe.data_offset + 11) == -1)
		return -1;
	
	return 0;
}

void close_device()
{
	close(fd);
}