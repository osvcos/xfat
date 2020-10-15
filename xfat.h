#ifndef XFAT_H
#define XFAT_H

#include "directory.h"
#include "types.h"

typedef struct {
	u32 current_entry;
	u64 data_offset;
	u32 next_entry;
} fat_entry;

s32 open_device(const char* dev);
u32 get_cluster_size();
s32 get_directory_entry(u32 *cluster_number, Directory *dir, u32 *offset);
s32 get_next_fat(u32 fat_index, fat_entry *fe);
u32 get_root_cluster();
s32 read_cluster(u32 cluster_number, u64 offset, void *data, u32 size);
s32 set_label(const char* label);
s32 write_to_bootsector(u32 offset, const void* data, u32 size);
void close_device();

#endif
