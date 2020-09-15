#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "types.h"

#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN    0x02
#define ATTR_SYSTEM    0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE   0x20

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

#endif
