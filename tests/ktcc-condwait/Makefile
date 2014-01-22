include ../shared.mk

EKCC_OPTS = --doglobalreg --dostackvars  --keepunused --automigrate
TEST=updatetest
SRC=testcondw.c
SRC2=testcondw2.c
CFLAGS += -lrt -lpthread
OBJ=$(patsubst %.c,%.o,$(SRC))
OBJ2=$(patsubst %.c,%.o,$(SRC2))
EKLIBTH  = ../../bin/libkitsune-threads.a

.PHONY: run-test
all: run-test

%.ktt: %.c
	$(EKCC) $(EKCC_OPTS) $(CFLAGS) $(EKINC) -c $< --doktsavetypes --typesfile-out=$@

.c.o:
	$(EKCC) $(EKCC_OPTS) $(CFLAGS) $(EKINC) -c $^

joined.ktt: testcondw2.ktt testcondw.ktt
	$(EKJOIN) $@ $^

dsu.c: joined.ktt $(TEST).xf
	$(EKGEN) $@ joined.ktt joined.ktt $(TEST).xf

$(TEST).so: $(OBJ)
	$(CC) $(CFLAGS_SHARED) -o $@ $^ $(EKLIBTH) -lpthread -lrt

$(TEST)2.so: $(OBJ2) dsu.c
	$(CC) $(CFLAGS_SHARED) -o $@ $^ $(EKLIBTH) -lpthread -lrt $(EKINC)

run-test: $(TEST).so $(TEST)2.so
	$(EKDRV) -b /tmp/condwait.txt $(shell pwd)/$(TEST).so &
	sleep 1
	../../bin/doupd `pidof driver` $(TEST)2.so
	sleep 1
	echo "\nkilling"
	killall driver

clean:
	rm -f *.o *.so *.ktt dsu.c
