include ../shared.mk

EKCC_OPTS = --doglobalreg --dostackvars  --keepunused --automigrate
TEST=ktcc-union
SRC=main.c
SRC2=mainv2.c
OBJ=$(patsubst %.c,%.o,$(SRC))
OBJ2=$(patsubst %.c,%.o,$(SRC2))

.PHONY: run-test
all: run-test

%.ktt: %.c
	$(EKCC) $(EKCC_OPTS) $(CFLAGS) $(EKINC) -c $< --doktsavetypes --typesfile-out=$@

.c.o:
	$(EKCC) $(EKCC_OPTS) $(CFLAGS) $(EKINC) -c $^

joined.ktt: $(TYPE_FILES)
	$(EKJOIN) $@ $^

dsu.c:  $(TEST).xf
	$(EKGEN) $@ main.ktt  mainv2.ktt $(TEST).xf

$(TEST).so: $(OBJ)
	$(CC) $(CFLAGS_SHARED) -o $@ $^ $(EKLIB)

$(TEST)2.so: $(OBJ2) dsu.c
	$(CC) $(CFLAGS_SHARED) -o $@ $^ $(EKLIB) $(EKINC)

run-test: $(TEST).so $(TEST)2.so
	$(EKDRV) $(shell pwd)/$(TEST).so $(shell pwd)/$(TEST)2.so

clean:
	rm -f *.o *.so *.ktt dsu.c
