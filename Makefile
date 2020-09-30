XFATLIB_CC      := gcc
XFATLIB_CFLAGS  := -O3
XFATLIB_LDFLAGS := -shared
XFATLIB_OBJS    := xfat.o
XFATLIB_OUT     := libxfat.so

all: $(XFATLIB_OBJS)
	gcc $(XFATLIB_LDFLAGS) $< -o $(XFATLIB_OUT)

utils: all
	make -C utils

%.o: %.c
	$(XFATLIB_CC) $(XFATLIB_CFLAGS) -c $<

clean:
	@rm -rf *.o
	@rm -rf $(XFATLIB_OUT)
	@make -C utils clean
