include ../shared.mk

EKCC_OPTS = 
TEST=updatetest
SRC=main.c main-2.c
TYPE_FILES=$(patsubst %.c,%.ktt,$(SRC))

.PHONY: run-test
all: run-test

%.ktt: %.c
	$(EKCC) $(EKCC_OPTS) $(CFLAGS) $(EKINC) -c $< --doktsavetypes --typesfile-out=$@

.c.o:
	$(EKCC) $(EKCC_OPTS) $(CFLAGS) $(EKINC) -c $^

dsu.c: $(TYPE_FILES) $(TEST).xf
	$(EKGEN) $@ $^

$(TEST).so: main.o 
	$(CC) $(CFLAGS_SHARED) -o $@ $^ $(EKLIB) 

$(TEST)2.so: main-2.o dsu.c
	$(CC) $(CFLAGS_SHARED) -o $@ $^ $(EKLIB) $(EKINC)

run-test: $(TEST).so $(TEST)2.so
	$(EKDRV) $(shell pwd)/$(TEST).so $(shell pwd)/$(TEST)2.so

clean:
	rm -f *.o *.so *.ktt dsu.c
