
SRC=mainv1.c mainv2.c pluginv1.c pluginv2.c

all: pluginv1.so pluginv2.so driver.o

driver.o:
	gcc driver.c -ldl -g -o  driver #-rdynamic
mainv1.o: 
	gcc -c -fPIC -g mainv1.c -o mainv1.o 
mainv2.o:                
	gcc -c -fPIC -g mainv2.c -o mainv2.o
mainv1.so: mainv1.o 
	gcc -shared -g -Wl,-soname,mainv1.so.1,--version-script=mainv1.ld -o mainv1.so  mainv1.o 
mainv2.so: mainv2.o
	gcc -shared -g -Wl,-soname,mainv2.so.1,--version-script=mainv2.ld -o mainv2.so  mainv2.o 
pluginv1.o: mainv1.so
	gcc -c -fPIC -g pluginv1.c -o pluginv1.o 
pluginv2.o: mainv2.so
	gcc -c -fPIC -g pluginv2.c -o pluginv2.o 
pluginv1.so: pluginv1.o 
	gcc -shared -g -Wl,-soname,pluginv1.so.1 -o pluginv1.so  pluginv1.o 
pluginv2.so: pluginv2.o
	ln -s mainv2.so ./libmainv2.so
	gcc -shared -g -Wl,-soname,pluginv2.so.1 -o pluginv2.so  pluginv2.o -L./ -lmainv2  


clean:
	rm -f *.o *.so *.cil.c *.i *.cil.i *.ktt driver
