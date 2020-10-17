#ifndef UTILS_H
#define UTILS_H

#include <sys/stat.h>

#include "directory.h"
#include "types.h"

s32 get_stat_from_directory(Directory *dir, struct stat *st);
s32 lookup_short_entry(const char *path, u32 *starting_cluster, Directory *dir);
s32 prettify_83_name(u8 *input_name, u8 *output_name);

#endif
