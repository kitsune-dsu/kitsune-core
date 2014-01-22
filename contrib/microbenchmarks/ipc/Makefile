
CC=gcc 
CFLAGS=-Wall -O2

all: uds-copy uds-copy-pull shm-1buf shm-2buf pipe-copy memcpy-simple memcpy-blocks

uds-copy: udscopy.c bench.h
	$(CC) $(CFLAGS) -o $@ $<

uds-copy-pull: udscopy-pull.c bench.h
	$(CC) $(CFLAGS) -o $@ $<

shm-1buf: shmcopy-1buf.c bench.h
	$(CC) $(CFLAGS) -o $@ $<

shm-2buf: shmcopy-2buf.c bench.h
	$(CC) $(CFLAGS) -o $@ $<

pipe-copy: pipecopy.c bench.h
	$(CC) $(CFLAGS) -o $@ $<

memcpy-simple: memcpy-simple.c bench.h
	$(CC) $(CFLAGS) -o $@ $<

memcpy-blocks: memcpy-blocks.c bench.h
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f *.o uds-copy shm-1buf shm-2buf pipe-copy memcpy-simple memcpy-blocks