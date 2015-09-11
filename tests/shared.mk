EKINC  = -I../../bin/
EKLIB  = ../../bin/lib/libkitsune.a
CFLAGS_SHARED  = -ggdb3 -Wall -ldl -shared -fPIC -u kitsune_init_inplace
CFLAGS = -ggdb3 -Wall -ldl -fPIC
EKDRV = ../../bin/bin/driver
EKCC = ../../bin/bin/ktcc
EKJOIN = ../../bin/bin/kttjoin
EKGEN = ../../bin/bin/xfgen
