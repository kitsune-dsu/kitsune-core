include ../shared.mk

EKCC_OPTS = --doglobalreg --dostackvars  --keepunused --automigrate
TEST=updatetest
V0_SRC=v0.c
V1_SRC=v1.c
V0_OBJ=$(patsubst %.c,%.o,$(V0_SRC))
V1_OBJ=$(patsubst %.c,%.o,$(V1_SRC))
V0_TYPE_FILES=$(patsubst %.c,%.ktt,$(V0_SRC))
V1_TYPE_FILES=$(patsubst %.c,%.ktt,$(V1_SRC))

.PHONY: run-test
all: run-test

%.ktt: %.c
	$(EKCC) $(EKCC_OPTS) $(CFLAGS) $(EKINC) -c $< --doktsavetypes --typesfile-out=$@

.c.o:
	$(EKCC) $(EKCC_OPTS) $(CFLAGS) $(EKINC) -c $^

v0-join.ktt: $(V0_TYPE_FILES)
	$(EKJOIN) $@ $^

v1-join.ktt: $(V1_TYPE_FILES)
	$(EKJOIN) $@ $^

dsu.c: v0-join.ktt v1-join.ktt $(TEST).xf
	$(EKGEN) $@ v0-join.ktt v1-join.ktt $(TEST).xf

$(TEST)0.so: $(V0_OBJ)
	$(CC) $(CFLAGS_SHARED) -o $@ $^ $(EKLIB)

$(TEST)1.so: $(V1_OBJ) dsu.c
	$(CC) $(CFLAGS_SHARED) -o $@ $^ $(EKLIB) $(EKINC)

run-test: $(TEST)0.so $(TEST)1.so
	$(EKDRV) $(TEST)0.so $(TEST)1.so

clean:
	rm -f *.o *.so *.ktt dsu.c
