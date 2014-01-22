include ../shared.mk

EKCC_OPTS = --doglobalreg --dostackvars  --keepunused --automigrate
TEST=updatetest
SRC=main.c other.c
OBJ=$(patsubst %.c,%.o,$(SRC))
OBJ1=$(patsubst %.c,%.o,main.c)
OBJ2=$(patsubst %.c,%.o,other.c)
TYPE_FILES=$(patsubst %.c,%.ekt,$(SRC))

.PHONY: run-test
all: run-test

%.ekt: %.c
	$(EKCC) $(EKCC_OPTS) $(CFLAGS) $(EKINC) -c $< --doktsavetypes --typesfile-out=$@

.c.o:
	$(EKCC) $(EKCC_OPTS) $(CFLAGS) $(EKINC) -c $^


#dsu.c: main.ekt other.ekt $(TEST).xf
#	$(EKGEN) $@ main.ekt other.ekt $(TEST).xf

main.so: $(OBJ1) 
	$(CC) $(CFLAGS_SHARED) -o $@ $^ $(EKLIB)

other.so: $(OBJ2) dsu.c
	$(CC) $(CFLAGS_SHARED) -o $@ $^ $(EKLIB) $(EKINC)

run-test: main.so other.so
	$(EKDRV) $(shell pwd)/main.so $(shell pwd)/other.so

clean:
	rm -f *.o *.so *.ekt #dsu.c
