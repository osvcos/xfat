# xfat

xfat is a library whose purpose is to give the basics to handle a FAT32 filesystem. To show this, it comes with a FUSE implementation.

## Building

To build the library, just type:

    make

To build the FUSE interface:

    make fuse

# Usage of the FUSE interface

    xfat-fuse <device> <mountpoint>
    
For example:

    xfat-fuse /dev/sdd1 /mnt
    
You might need to specify the location of libxfat. To do so, you can use the LD_LIBRARY_PATH environment variable:

    LD_LIBRARY_PATH=/path/to/libxfat.so xfat-fuse /dev/sdd1 /mnt

# Miscellaneous programs

xfat comes with example programs that shows how to use its API. To build them:

    make utils

These programs also require to use libxfat.so.
