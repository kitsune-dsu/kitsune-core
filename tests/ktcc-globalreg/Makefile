include ../shared.mk

TEST=globalreg
SRC=main.c other.c
OBJ=$(patsubst %.c,%.o,$(SRC))

EKCC_OPTS = --keepunused --doglobalreg --dostackvars

.PHONY: run-test
all: run-test

.c.o:
	$(EKCC) $(EKCC_OPTS) $(EKINC) $(CFLAGS) -c $@ $<

$(TEST).so: $(OBJ)
	$(CC) $(CFLAGS_SHARED) -o $@ $^ $(EKLIB)

$(TEST)2.so: $(TEST).so
	cp $^ $@

run-test: $(TEST).so $(TEST)2.so
	$(EKDRV) $(shell pwd)/$(TEST).so $(shell pwd)/$(TEST)2.so

clean:
	rm -f *.o *.so
