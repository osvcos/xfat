# xfat

xfat is a library whose purpose is to give the basics to handle a FAT32 filesystem. To show this, it comes with a FUSE implementation.

## Building

To build the library, just type:

    make

And to build the FUSE interface:

    make fuse

xfat comes with example programs that shows how to use its API. To build them:

    make utils
