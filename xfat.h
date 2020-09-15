#ifndef XFAT_H
#define XFAT_H

#include "types.h"

typedef struct {
	u32 current_entry;
	u32 data_offset;
	u32 next_entry;
} fat_entry;

s32 open_device(const char* dev);
s32 get_next_fat(u32 fat_index, fat_entry *fe);
u32 get_root_cluster();
s32 set_label(const char* label);
void close_device();

#endif
