#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "types.h"

#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN    0x02
#define ATTR_SYSTEM    0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE   0x20

/* Attribute only applied to LFN entries */
#define ATTR_LONG_NAME (ATTR_READ_ONLY | \
    ATTR_HIDDEN | \
    ATTR_SYSTEM | \
    ATTR_VOLUME_ID)

#define MAX_LFN_LENGTH 255

typedef struct __attribute__((packed)) {
	u8  name[11];
	u8  attributes;
	u8  reserved;
	u8  time_tenth;
	u16 creation_time;
	u16 creation_date;
	u16 last_access_date;
	u16 first_clus_hi;
	u16 last_mod_time;
	u16 last_mod_date;
	u16 first_clus_low;
	u32 file_size;
} Directory;

typedef struct __attribute__((packed)) {
    u8  ordinal;
    u8  name1[10];
    u8  attributes;
    u8  type;
    u8  checksum;
    u8  name2[12];
    u16 first_clus_low;
    u8  name3[4];
} lfn_entry;

typedef struct {
    u8 long_name[MAX_LFN_LENGTH];
    Directory dir;
} dir_info;

s32 get_directory_entry(u32 *cluster_number, dir_info *di, u32 *offset);

#endif
