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
static u32 bytes_per_sector;
static u64 data_region_offset;
static u32 cluster_size;
static u32 root_cluster;
static u32 backup_boot_sector_cluster;

static u32 is_forbidden_char(u8 c)
{
    u8 forbidden_chars[] = {
        0x22, 0x2a, 0x2b, 0x2c,
        0x2e, 0x2f, 0x3a, 0x3b,
        0x3c, 0x3d, 0x3e, 0x3f,
        0x5b, 0x5c, 0x5d, 0x7c
    };
    u32 low  = 0;
    u32 high = sizeof(forbidden_chars) - 1;
    u32 mid  = 0;

    while(low <= high)
    {
        mid = (low + ((high - low) / 2));

        if(forbidden_chars[mid] > c)
        {
            high = mid - 1;
            continue;
        }
        if(forbidden_chars[mid] < c)
        {
            low = mid + 1;
            continue;
        }
        if(forbidden_chars[mid] == c)
            return 1;
    }

    return 0;
}

static void validate_83_name(const u8 *input_name, u32 input_name_size, u8 *output_name)
{
    u8  new_name[11];

    memset(new_name, 0x20, 11);

    for(s32 i = 0; i < input_name_size; i++)
    {
        if(islower(input_name[i]))
            new_name[i] = toupper(input_name[i]);

        if(is_forbidden_char(input_name[i]))
            new_name[i] = 0x20;
    }

    memcpy(output_name, new_name, 11);
}

s32 open_device(const char* dev)
{
    CBPB common_bpb;
    FAT32BPB fat32_bpb;
    u16 fat_in_use = 0;

    memset(&common_bpb, 0, sizeof(CBPB));
    memset(&fat32_bpb, 0, sizeof(FAT32BPB));

    fd = open(dev, O_RDWR | O_SYNC | O_RSYNC);

    if(fd < 0)
        return -1;

    if(pread(fd, &common_bpb, sizeof(CBPB), 0) == -1)
		return -1;

    if(pread(fd, &fat32_bpb, sizeof(FAT32BPB), sizeof(CBPB)) == -1)
        return -1;

    if(fat32_bpb.ext_flags & 0x0080)
        fat_in_use = (fat32_bpb.ext_flags & 0x000F);

	bytes_per_sector = common_bpb.bytes_per_sector;

	fat_region_offset = (common_bpb.reserved_sectors * bytes_per_sector)
        + (fat_in_use * fat32_bpb.fat_size_32 * bytes_per_sector * common_bpb.fat_count);

	data_region_offset = (common_bpb.reserved_sectors * bytes_per_sector)
        + ((fat32_bpb.fat_size_32 * bytes_per_sector) * common_bpb.fat_count);

	cluster_size = bytes_per_sector * common_bpb.sectors_per_cluster;
	root_cluster = fat32_bpb.root_cluster;
	backup_boot_sector_cluster = fat32_bpb.boot_sector_copy;

	return 0;
}

s32 get_next_fat(u32 fat_index, fat_entry *fe)
{
    u32 next_fat = 0;
    u32 current_fat = fat_region_offset + (4 * fat_index);

	fe->current_entry = fat_index;
	
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
    u8 new_label[11];
	
    memset(&dir, 0, sizeof(Directory));
    memset(&fe, 0, sizeof(fat_entry));
    memset(&dt, 0, sizeof(struct fat_datetime));
    memset(new_label, 0, 11);

    validate_83_name(label, strnlen(label, 11), new_label);

    if(get_next_fat(root_cluster, &fe) == -1)
        return -1;
    
    if(pread(fd, &dir, sizeof(Directory), fe.data_offset) == -1)
        return -1;
    
    if(fat_getdatetime(&dt) == -1)
        return -1;
    
    memcpy(dir.name, new_label, 11);
    
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
	
	if(write_to_bootsector(label_offset, new_label, 11) == -1)
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
