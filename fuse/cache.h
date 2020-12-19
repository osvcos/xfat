#ifndef CACHE_H
#define CACHE_H

#include "types.h"

typedef struct {
    u32 initial_cluster;
    u64 offset;
    u32 final_cluster;
} cache_entry;

s32  cache_add(u32 initial_cluster, u64 offset, u32 final_cluster);
void cache_destroy();
void cache_init();
s32  cache_lookup(u32 initial_cluster, u64 offset, cache_entry *ce);

#endif
