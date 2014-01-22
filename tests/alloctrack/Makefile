include ../shared.mk

TEST=staticvars
SRC=main.c other.c dsu.c
OBJ=$(patsubst %.c,%.o,$(SRC))

.PHONY: run-test
all: run-test

.c.o:
	$(CC) $(CFLAGS) $(EKINC) -c $<

$(TEST).so: $(OBJ)
	$(CC) $(CFLAGS_SHARED) -o $@ $^ $(EKLIB)

$(TEST)2.so: $(TEST).so
	cp $^ $@

run-test: $(TEST).so $(TEST)2.so
	$(EKDRV) $(shell pwd)/$(TEST).so $(shell pwd)/$(TEST)2.so

clean:
	rm -f *.o *.so
