KITSUNE_PATH = ../../../../../../bin

KITSUNE_LDFLAGS =  -L$(KITSUNE_PATH) -lkitsune -ldl
KITSUNE_CFLAGS = -I$(KITSUNE_PATH) -fPIC

KTCC = $(KITSUNE_PATH)/ktcc
XFGEN = $(KITSUNE_PATH)/xfgen

PREV_KTT = ../v0/keyvalueserver.ktt

PROG = keyvalueserver
SOURCE = keyvalueserver.c
XF = keyvalueserver.xf
CFLAGS = -O2 -fomit-frame-pointer -g $(KITSUNE_CFLAGS)

all: $(PROG) $(PROG).so

dsu.c: keyvalueserver.o
	$(XFGEN) $@ $(PREV_KTT) keyvalueserver.ktt $(XF)

dsu.o: dsu.c
	$(CC) $(CFLAGS) -c $^

%.o: %.c
	$(KTCC) $(CFLAGS) -c $^

$(PROG): keyvalueserver.o
	$(CC) -o $@ $^ $(KITSUNE_LDFLAGS)

$(PROG).so: keyvalueserver.o dsu.o
	$(CC) -shared -o $@ $^ $(KITSUNE_LDFLAGS)

clean:
	rm *.o || true
	rm $(PROG) || true
	rm *.ktt || true
	rm $(PROG).so || true
	-rm dsu.c