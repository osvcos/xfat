# xfat

xfat is a library whose purpose is to give the basics to handle a FAT32 filesystem. To show this, it comes with a FUSE implementation.

## Building

To build the library, just type:

    make

To build the FUSE interface:

    make fuse

## The FUSE interface

xfat comes with xfat-fuse, a program that uses both the libxfat and libfuse to create an utility that mounts a block device to the specified mountpoint, just like the traditional mount would do. Currently, the FUSE interface is capable of:

* Mount a file/block device
* List the contents of the root directory
* Traverse over the filesystem tree
* Read files
* Provide date/time information for files/directories
* Display filenames in the long filename format (LFN)
* Provide filesystem information (filesystem size, available size, block size)

In order to use it, just type:

    xfat-fuse <device> <mountpoint>
    
For example:

    xfat-fuse /dev/sdd1 /mnt
    
You will need to specify the location of libxfat. To do so, you can use the LD_LIBRARY_PATH environment variable:

    LD_LIBRARY_PATH=/path/to/libxfat/directory xfat-fuse /dev/sdd1 /mnt

## Miscellaneous programs

xfat comes with example programs that shows how to use its API. To build them:

    make utils

These programs also require to use libxfat.so.
