#include <stdio.h>

#include "xfat.h"

int main(int argc, char *argv[])
{
    if(argc != 3)
    {
        printf("usage: fatlabel <device> <label>\n");
        return -1;
    }

    if(open_device(argv[1]) == -1)
    {
        printf("Error openning device\n");
        return -1;
    }

    if(set_label(argv[2]) == -1)
    {
        printf("Could not set label\n");
        return -1;
    }

    close_device();

    return 0;
}
