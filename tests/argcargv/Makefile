include ../shared.mk

TEST 	 = argcargv
SRC	     = $(TEST).c

all: run-test

$(TEST): $(SRC)
	$(CC) $(EKINC) $(CFLAGS) -shared -fPIC -o $@.so $@.c $(EKLIB)

run-test: $(TEST)
	cp $(TEST).so $(TEST)2.so
	$(EKDRV) $(shell pwd)/$(TEST).so $(shell pwd)/$(TEST)2.so b c

depend:
	gcc $(IPATH) -MM $(SRC) 

clean:
	-$(RM) $(TEST).so $(TEST)2.so *~
