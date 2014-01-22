ITREE_DIR = ../contrib/interval_tree
ITREE_LIB = $(ITREE_DIR)/interval.o

UTHASH_DIR = ../contrib/uthash

CC=gcc 
CFLAGS=-Wall -Werror -ggdb -fPIC -I. -I$(ITREE_DIR) -I../contrib/uthash -D_XOPEN_SOURCE=500 -ldl -lrt

DRV_NAME = driver

LIB_SRC = kitsune.c addresscheck.c stackvars.c registervars.c transform.c bench.c log.c heaptrack.c vmareas.c alloctrack.c 
LIB_OBJ = ${LIB_SRC:.c=.o}
LIB_NAME = libkitsune.a

LIBTHREAD_SRC = $(LIB_SRC) ktthreads.c
LIBTHREAD_OBJ = ${LIBTHREAD_SRC:.c=-thd.o} 
LIBTHREAD_NAME = libkitsune-threads.a

DRV_SRC = driver.c 	
HELPERS = doupd

all: $(DRV_NAME) $(LIB_NAME) $(LIBTHREAD_NAME) $(HELPERS)

$(DRV_NAME): $(DRV_SRC)
	$(CC) $(CFLAGS) -o $@ $^ -ldl

$(LIB_OBJ): %.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(LIBTHREAD_OBJ): %-thd.o: %.c
	$(CC) -c $(CFLAGS) -D ENABLE_THREADING $< -o $@

$(LIB_NAME): $(LIB_OBJ)
	ar rcs $@ $(LIB_OBJ) $(ITREE_LIB) 

$(LIBTHREAD_NAME): $(LIBTHREAD_OBJ)
	ar rcs $@ $(LIBTHREAD_OBJ) $(ITREE_LIB)

clean:
	rm -f *.o *.a $(DRV_NAME) $(HELPERS) *.dSYM

doupd.sh:

doupd: doupd.sh
	cp doupd.sh doupd
