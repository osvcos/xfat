XFATLIB_CC      := gcc
XFATLIB_CFLAGS  := -O3
XFATLIB_LDFLAGS := -shared
XFATLIB_OBJS    := xfat.o datetime.o
XFATLIB_OUT     := libxfat.so

all: $(XFATLIB_OBJS)
	$(XFATLIB_CC) $(XFATLIB_LDFLAGS) $(XFATLIB_OBJS) -o $(XFATLIB_OUT)

utils: all
	make -C utils

fuse: all
	make -C fuse

%.o: %.c
	$(XFATLIB_CC) $(XFATLIB_CFLAGS) -c $<

clean:
	@rm -rf *.o
	@rm -rf $(XFATLIB_OUT)
	@make -C utils clean
	@make -C fuse clean
