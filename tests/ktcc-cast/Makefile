include ../shared.mk

EKCC_OPTS = --doglobalreg --dostackvars  --keepunused --save-temps
TEST=updatetest
SRC=main.c 
OBJ=$(patsubst %.c,%.o,$(SRC))
TYPE_FILES=$(patsubst %.c,%.ktt,$(SRC))

.PHONY: run-test
all: run-test

%.ktt: %.c
	$(EKCC) $(EKCC_OPTS) $(CFLAGS) $(EKINC) -c $< --doktsavetypes --typesfile-out=$@

.c.o:
	$(EKCC) -Werror $(EKCC_OPTS) $(CFLAGS) -Wall -Werror $(EKINC) -c $^

joined.ktt: $(TYPE_FILES)
	$(EKJOIN) $@ $^

dsu.c: joined.ktt $(TEST).xf
	$(EKGEN) $@ joined.ktt joined.ktt $(TEST).xf

$(TEST).so: $(OBJ)
	$(CC) $(CFLAGS_SHARED) -o $@ $^ $(EKLIB)

$(TEST)2.so: $(OBJ) dsu.c
	$(CC) $(CFLAGS_SHARED) -o $@ $^ $(EKLIB) $(EKINC)

run-test: $(TEST).so $(TEST)2.so
	$(EKDRV) $(shell pwd)/$(TEST).so $(shell pwd)/$(TEST)2.so

clean:
	rm -f *.o *.so *.ktt dsu.c *.cil.c *.i
