KITSUNE_PATH = ../../../../bin

SOURCE = ex1.c
CFLAGS = -O2 -fomit-frame-pointer


%.o: %.c
	$(CC) $(CFLAGS) -c $^

ex1: ex1.o
	$(CC) -o $@ $^
