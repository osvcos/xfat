XFATLIB_CC      := gcc
XFATLIB_CFLAGS  := -O3
XFATLIB_LDFLAGS := -shared
XFATLIB_OBJS    := xfat.o
XFATLIB_OUT     := libxfat.so

all: $(XFATLIB_OBJS)
	gcc $(XFATLIB_LDFLAGS) $< -o $(XFATLIB_OUT)

utils: all
	make -C utils

xfat.o: xfat.c
	$(XFATLIB_CC) $(XFATLIB_CFLAGS) -c $<
