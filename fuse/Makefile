CC      := gcc
LDFLAGS := $(shell pkg-config --libs fuse) -L../ -lxfat
CFLAGS  := $(shell pkg-config --cflags fuse)
OBJECTS := xfat-fuse.o
OUTPUT  := xfat-fuse

all: xfat $(OUTPUT)

xfat:
	make -C ../

$(OUTPUT): $(OBJECTS)
	$(CC) $(LDFLAGS) $< -o $@

%.o : %.c
	$(CC) $(CFLAGS) -c $<
	
clean:
	@rm -rf $(OBJECTS)
	@rm -rf $(OUTPUT)
