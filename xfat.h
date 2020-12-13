#ifndef XFAT_H
#define XFAT_H

#include "directory.h"
#include "types.h"

/*
 * This structure is used by get_next_entry() to
 * store information related to its fat_index parameter
 */
typedef struct {
    /* The fat_index parameter of get_next_entry() */
    u32 current_entry;
    
    /* The offset in bytes of the start of the cluster
     * represented by fat_index */
    u64 data_offset;
    
    /* The entry in the File Allocation Table pointed to
     * by the fat_index parameter*/
    u32 next_entry;
} fat_entry;

/*
 * Closes the file descriptor pointed to by the dev
 * parameter of open_device()
 */
void close_device();

/*
 * Traverses the File Allocation Table and looks
 * for an empty entry (0x00000000) and, when hit,
 * it returns the position of that free entry in
 * the free_entry parameter.
 * 
 * @free_entry: a pointer to a variable where to
 * store the position of the first free entry in
 * the File Allocation Table.
 */
s32  find_free_fat_entry(u32 *free_entry);

/*
 * Gets the cluster size of the device from the
 * superblock
 */
u32  get_cluster_size();

/*
 * Gets the total clusters of the device from the
 * File System Info structure
 */
u32  get_data_clusters_count();

/*
 * Gets the free cluster count of the device from
 * the File System Info Structure
 */
u32  get_free_clusters_count();

/*
 * Given a fat index in the File Allocation Table,
 * it returns: the FAT index itself, the FAT entry
 * it points to and the offset, in bytes, of the
 * start of the cluster it represents. All this
 * information is returned in the fe parameter.
 * 
 * @fat_index: an index in the FAT Allocation
 * Table.
 * 
 * @fe: a structure that contains the fat_index
 * parameter, the FAT index it points to, and
 * the offset of the start of the cluster that
 * fat_index represents.
 */
s32  get_next_entry(u32 fat_index, fat_entry *fe);

/*
 * Gets the cluster that holds the root directory
 * from the superblock
 */
u32  get_root_cluster();

/*
 * This function should be called after any of the
 * others function. It gets information about the
 * device pointed to by dev that will be used by
 * the other functions. Currently, this function
 * does not perform any validation, so it assumes
 * the device is a legit FAT32 filesystem.
 * 
 * @dev: a string that represents a device
 * (or file) containing a FAT32 filesystem
 */
s32  open_device(const char* dev);

/*
 * Reads the File System Info structure and
 * sets/updates the internal variables used in the
 * library
 */
s32  read_fsi();

/*
 * Reads the cluster represented by cluster_number,
 * starting at offset bytes, until size bytes into
 * the data parameter. Internally, read_cluster()
 * uses get_next_entry() to find the offset of the
 * cluster_number cluster. The function performs
 * cluster boundary validation.
 * 
 * @cluster_number: the index of the cluster in the
 * File Allocation Table to be read.
 * 
 * @offset: the offset in bytes from the start of the
 * cluster.
 * 
 * @data: the start address of a buffer that will be
 * used to store the read data.
 * 
 * @size: the lenghth of data to be read.
 */
s32  read_cluster(u32 cluster_number, u64 offset, void *data, u32 size);

/*
 * Sets the label of the filesystem. Validation of the
 * given label is performed to conform the 8.3 name
 * format.
 * 
 * @label: the new label of the filesystem.
 */
s32  set_label(const char* label);

/*
 * Writes size bytes in the cluster represented by
 * cluster_number, starting at offset into data.This 
 * function is not fully implemented by now.
 * 
 * @cluster_number: The cluster index in the File
 * Allocation Table.
 * 
 * @offset: The starting position from the begining of
 * the cluster.
 * 
 * @data: the buffer where to store the data that was
 * read.
 * 
 * @size: the amount of data to be read.
 */
s32  write_to_cluster(u32 cluster_number, u64 offset, void *data, u32 size);

/*
 * Writes new_value in the position fat_index of the
 * File Allocation Table.
 * 
 * @fat_index: an index in the File Allocation Table
 * where to write new_value.
 * 
 * @new_value: the value to write in the index represented
 * by fat_index.
 */
s32  write_to_fat_entry(u32 fat_index, u32 new_value);

/*
 * Writes data into the FIle System Information structure, at
 * offset (from the begining of the structure), and with a length
 * of size.
 * 
 * @offset: the offset in bytes from the begining of the structure.
 * 
 * @data: the buffer to be written.
 * 
 * @size: the length of the buffer to be written.
 */
s32  write_to_fsi(u64 offset, const void* data, u32 size);

/*
 * Writes data into the bootsector, at offset (from the begining 
 * of the structure), and with a length of size.
 * 
 * @offset: the offset in bytes from the begining of the structure.
 * 
 * @data: the buffer to be written.
 * 
 * @size: the length of the buffer to be written.
 */
s32  write_to_bootsector(u32 offset, const void* data, u32 size);

#endif
