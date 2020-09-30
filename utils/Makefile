CC      := gcc
CCFLAGS := -I../
LDFLAGS := -L../ -lxfat
FATLABEL_OBJECTS := fatlabel.o
TESTS := fatlabel

all: xfat $(TESTS)
.PHONY: all

xfat:
	@make -C ../

fatlabel: $(FATLABEL_OBJECTS)
	$(CC) $(LDFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CCFLAGS) -c $<

clean:
	@rm -rf *.o
	@rm -rf $(TESTS)
