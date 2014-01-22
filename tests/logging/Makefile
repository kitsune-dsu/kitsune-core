include ../shared.mk

TESTS = logging standalone

all: $(TESTS) run-test

.PHONY: logging run-test

standalone:
	gcc $(CFLAGS) $(EKINC) -o $@ $@.c  $(EKLIB)

logging.so: logging.c
	$(CC) $(CFLAGS_SHARED) $(EKINC) -o $@ $^ $(EKLIB)

logging2.so: logging.so
	cp $^ $@

logging: logging.so logging2.so

run-test: $(TESTS) logging-test standalone-test

logging-test: logging
	$(EKDRV) $(shell pwd)/logging.so $(shell pwd)/logging2.so

standalone-test: standalone
	./standalone

clean:
	-$(RM) $(TESTS) *.o *.so *~
