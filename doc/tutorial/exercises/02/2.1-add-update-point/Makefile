KITSUNE_PATH = ../../../../../bin

KITSUNE_LDFLAGS =  -L$(KITSUNE_PATH) -lkitsune -ldl
KITSUNE_CFLAGS = -I$(KITSUNE_PATH) -fPIC

KTCC = $(KITSUNE_PATH)/ktcc

PROG = keyvalueserver
SOURCE = keyvalueserver.c
CFLAGS = -O2 -fomit-frame-pointer $(KITSUNE_CFLAGS)

all: $(PROG) $(PROG).so

%.o: %.c
	$(KTCC) $(CFLAGS) -c $^

$(PROG): keyvalueserver.o
	$(CC) -o $@ $^ $(KITSUNE_LDFLAGS)

$(PROG).so: keyvalueserver.o
	$(CC) -shared -o $@ $^ $(KITSUNE_LDFLAGS)

clean:
	rm *.o || true
	rm $(PROG) || true
	rm *.ktt || true
	rm $(PROG).so || true