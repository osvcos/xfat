#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "cache.h"

#define MAX_CACHE_ENTRIES 2000

#define COMPARE(x, op, y) \
    ((x.initial_cluster op y.initial_cluster) \
    && (x.offset op y.offset))

static cache_entry entries[MAX_CACHE_ENTRIES];
static s32 current_index;
static pthread_mutex_t mutex;

static void __insert(cache_entry entry, s32 index)
{
    if(index == 0)
    {
        entries[index] = entry;
        return;
    }
    
    while(index > 0)
    {
        if(COMPARE(entry, ==, entries[index - 1]))
        {
            break;
        }
        else if(COMPARE(entry, <=, entries[index - 1]))
        {
            entries[index] = entries[index - 1];
            entries[index - 1] = entry;
            __insert(entries[index - 1], index - 1);
            break;
        }
        else
        {
            entries[index] = entry;
            break;
        }
    }
}

s32  cache_add(u32 initial_cluster, u64 offset, u32 final_cluster)
{
    cache_entry ce;
    
    memset(&ce, 0, sizeof(cache_entry));
    
    pthread_mutex_lock(&mutex);
    printf("cache_add(initial_cluster=%u, offset=%llu, final_cluster=%u)\n",
           initial_cluster, offset, final_cluster);
    
    ce.initial_cluster = initial_cluster;
    ce.offset = offset;
    ce.final_cluster = final_cluster;
    
    __insert(ce, current_index);
    
    if(current_index < (MAX_CACHE_ENTRIES - 1))
        current_index += 1;
    
#ifdef DEBUG
    for(s32 i = 0; i <= current_index; i++)
    {
        printf("initial_cluster=%u, offset=%llu, final_cluster=%u\n",
               entries[i].initial_cluster, entries[i].offset, entries[i].final_cluster);
    }
#endif

    pthread_mutex_unlock(&mutex);
    
    return 0;
}

void cache_destroy()
{
    pthread_mutex_destroy(&mutex);
}

void cache_init()
{
    current_index = 0;
    
    memset(entries, 0, (MAX_CACHE_ENTRIES * sizeof(cache_entry)));
    pthread_mutex_init(&mutex, NULL);
}

s32  cache_lookup(u32 initial_cluster, u64 offset, cache_entry *ce)
{
    s32 low  = 0;
    s32 high = current_index;
    s32 mid  = 0;
    cache_entry centry;
    
    centry.initial_cluster = initial_cluster;
    centry.offset = offset;
    
    printf("cache_lookup(initial_cluster=%u, offset=%llu)\n",
           initial_cluster, offset);

    while(low <= high)
    {
        mid = (low + ((high - low) / 2));
        
        if(COMPARE(entries[mid], ==, centry))
        {
            printf("cache_lookup: entry found\n");
            centry.final_cluster = entries[mid].final_cluster;
            memcpy(ce, &centry, sizeof(cache_entry));
            return 0;
        }
        else if(COMPARE(entries[mid], <=, centry))
        {
            low = mid + 1;
        }
        else
        {
            high = mid - 1;
        }
    }
    
    printf("cache_lookup: entry not found\n");
    return -1;
}
