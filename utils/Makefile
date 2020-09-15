CC      := gcc
CCFLAGS := -I../
LDFLAGS := -L../ -lxfat

all: fatlabel
	make -C ../.

fatlabel: fatlabel.o
	$(CC) $(LDFLAGS) fatlabel.o -o fatlabel

fatlabel.o: fatlabel.c
	$(CC) $(CCFLAGS) -c fatlabel.c

clean:
	@rm -rf *.o
	@rm -rf fatlabel
