/*
 * Structures present in the reserved region
 * of the filesystem
 */
#ifndef RESERVED_H
#define RESERVED_H

#include "types.h"

typedef struct __attribute__((packed)) {
	u8  jump_boot[3];
	u8  oem_name[8];
	u16 bytes_per_sector;
	u8  sectors_per_cluster;
	u16 reserved_sectors;
	u8  fat_count;
	u16 root_entries_count;
	u16 total_sectors_16;
	u8  media;
	u16 fat_size_16;
	u16 sectors_per_track;
	u16 number_of_heads;
	u32 hidden_sectors;
	u32 total_sectors_32;
} CBPB;

typedef struct __attribute__((packed)) {
	u32 fat_size_32;
	u16 ext_flags;
	u16 fs_version;
	u32 root_cluster;
	u16 fs_info_cluster;
	u16 boot_sector_copy;
	u8  reserved[12];
	u8  drive_number;
	u8  reserved1;
	u8  boot_signature;
	u32 volume_serial_number;
	u8  volume_label[11];
	u8  file_system_type[8];
	u8  reserved2[420];
	u16 signature;
} FAT32BPB;

typedef struct __attribute__((packed)) {
	u32 lead_signature;
	u8  reserved1[480];
	u32 struct_signature;
	u32 free_cluster_count;
	u32 first_free_cluster;
	u8  reserved2[12];
	u32 trail_signature;
} FSINFO;
#endif
