#include <fcntl.h>
#include <unistd.h>

#include "xfat.h"

static u32 data_region_offset;
static u32 fat_region_offset;
static u32 bytes_per_sector;
static s32 fd;

s32 open_device(const char* dev)
{
	return 0;
}

void close_device()
{
	close(fd);
}
