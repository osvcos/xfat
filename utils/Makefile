CC      := gcc
CCFLAGS := -I../
LDFLAGS := -L../ -lxfat
FATLABEL_OBJECTS := fatlabel.o
LSROOT_OBJECTS := lsroot.o
LSROOT2_OBJECTS := lsroot2.o
TESTS := fatlabel lsroot lsroot2

all: xfat $(TESTS)
.PHONY: all

xfat:
	@make -C ../

fatlabel: $(FATLABEL_OBJECTS)
	$(CC) $(LDFLAGS) $< -o $@

lsroot: $(LSROOT_OBJECTS)
	$(CC) $(LDFLAGS) $< -o $@

lsroot2: $(LSROOT2_OBJECTS)
	$(CC) $(LDFLAGS) $< -o $@
%.o: %.c
	$(CC) $(CCFLAGS) -c $<

clean:
	@rm -rf *.o
	@rm -rf $(TESTS)
