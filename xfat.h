#ifndef XFAT_H
#define XFAT_H

#include "directory.h"
#include "types.h"

typedef struct {
    u32 current_entry;
    u64 data_offset;
    u32 next_entry;
} fat_entry;

void close_device();
s32  find_free_fat_entry(u32 *free_entry);
u32  get_cluster_size();
u32  get_data_clusters_count();
u32  get_free_clusters_count();
s32  get_next_entry(u32 fat_index, fat_entry *fe);
u32  get_root_cluster();
s32  open_device(const char* dev);
s32  read_fsi();
s32  read_cluster(u32 cluster_number, u64 offset, void *data, u32 size);
s32  set_label(const char* label);
s32  write_to_cluster(u32 cluster_number, u64 offset, void *data, u32 size);
s32  write_to_fat_entry(u32 fat_index, u32 new_value);
s32  write_to_fsi(u64 offset, const void* data, u32 size);
s32  write_to_bootsector(u32 offset, const void* data, u32 size);

#endif
