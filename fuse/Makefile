CC      := gcc
LDFLAGS := $(shell pkg-config --libs fuse) -L../ -lxfat
CFLAGS  := $(shell pkg-config --cflags fuse) -I../
OBJECTS := xfat-fuse.o utils.o
OUTPUT  := xfat-fuse

ifdef DEBUG
ifeq ($(DEBUG),1)
CFLAGS +=  -DDEBUG
endif
endif

all: xfat $(OUTPUT)

xfat:
	make -C ../

$(OUTPUT): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

%.o : %.c
	$(CC) $(CFLAGS) -c $<
	
clean:
	@rm -rf $(OBJECTS)
	@rm -rf $(OUTPUT)
