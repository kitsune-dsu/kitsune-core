KITSUNE_PATH = ../../../../../../bin

KITSUNE_LDFLAGS =  -L$(KITSUNE_PATH) -lkitsune -ldl
KITSUNE_CFLAGS = -I$(KITSUNE_PATH) -fPIC -include kitsune.h

KTCC = $(KITSUNE_PATH)/ktcc
XFGEN = $(KITSUNE_PATH)/xfgen
KTTJOIN = $(KITSUNE_PATH)/kttjoin

PREV_KTT = prog.ktt

PROG = keyvalueserver
SOURCE = keyvalueserver.c tree.c
XF = keyvalueserver.xf
CFLAGS = -O2 -fomit-frame-pointer -g $(KITSUNE_CFLAGS)

all: $(PROG) $(PROG).so

prog.ktt: keyvalueserver.o tree.o
	$(KTTJOIN) $@ $(subst .o,.ktt,$^) 

dsu.c: prog.ktt keyvalueserver.xf
	$(XFGEN) $@ $(PREV_KTT) $^ 

dsu.o: dsu.c
	$(CC) $(CFLAGS) -c $^

%.o: %.c
	$(KTCC) $(CFLAGS) -c $^

$(PROG): keyvalueserver.o tree.o dsu.o
	$(CC) -o $@ $^ $(KITSUNE_LDFLAGS)

$(PROG).so: keyvalueserver.o tree.o dsu.o
	$(CC) -shared -o $@ $^ $(KITSUNE_LDFLAGS)

clean:
	rm *.o || true
	rm $(PROG) || true
	rm *.ktt || true
	rm $(PROG).so || true
	-rm dsu.c