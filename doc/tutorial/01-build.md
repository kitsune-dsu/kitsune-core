Building and Running Programs with Kitsune
==========================================

Building
--------

The Kitsune runtime expects your program will be in the form of a shared library. You will have to modify your Makefiles to produce these shared objects. In general, this rule will look similar to the rule that produces a binary.

By default, Kitsune requires you to build all C files with the Kitsune translator. This is a simple wrapper around GCC that makes it much easier to enable dynamic updating (it is possible to use Kitsune without using the Kitsune translator, but this makes adding support for updating tedious and error-prone).

To demonstrate, here is a line to compile C files to object files using Kitsune:

`%.o: %.c`
`	ktcc $(CFLAGS) -fPIC -I/path/to/kitsune -include kitsune.h -c $?`

Including kitsune.h on the command line isn't strictly necessary, but since the Kitsune runtime uses Kitsune APIs in every file with state that might be updated, it's convenient and prevents you from having an extra line in each file. (For reference, `-I` designates a directory that GCC should search for header files.)

The compiler flag `-fPIC` causes the object code generated to be position-independent, which is necessary for the linking these objects together into a shared object as below:

`program.so: $(OBJS)`
`	gcc -shared -o $@ $^ -L/path/to/kitsune -lkitsune  -ldl`

The `-L` flag is analogous to the `-I` flag for the linker. `-lkitsune` links your program with the Kitsune runtime library.

Even if your program doesn't use the dynamic linking library, the Kitsune runtime requires it. Since the Kitsune runtime isn't compiled to a shared object (so that one version of it will exist in each version of your program), it's necessary to include -ldl on the command line for your program's shared object.

Also, notice that the Kitsune library is referenced on the command line after the program objects, and `-ldl` appears after the Kitsune runtime library. This is somehow important. If you're curious, take it up with the GCC developers.

Exercise 1.0: Build and Run a program with Kitsune
------------------------------------------------

Before you begin writing dynamically updatable programs, you need to be able to build and execute those programs. This exercise will familiarize you with the Kitsune compiler and driver program.

You need to modify the Makefile to add another target, `ex1.so`. This target will be loaded and executed by the Kitsune driver as described in the section of the Kitsune tutorial entitled *Running Programs with Kitsune.*

*(Hint: the path to the Kitsune installation is stored in the variable at the first line of the Makefile.)*

Running and Updating Kitsune Programs
-------------------------------------

Kitsune programs are executed with a small `driver` program, installed with the binary name `driver`. This driver is a short C program, and is the only part of a running Kitsune program that cannot be updated.

To run a program under the Kitsune driver, simply invoke `driver` with the path to your shared object as the first argument:

	driver ./foo.so
	
If you successfully completed exercise 0.0, you should see the following when running Kitsune under the driver program:

	$ driver ex1.so
	The process id is (2476)
	Hello, World!
	$ 

The driver program prints the PID of the Kitsune process upon launch. This is to facilitate updating with Kitsune's `doupd` utility. 

`doupd` is a small program that takes two arguments: the process ID of the process to be updated, and the path to the shared object for the next version, as such:

	$ doupd 2476 ex1-update.so

If doupd is passed the address of the currently existing shared object, `dlopen` will not map the existing file into another section of memory, and while transformation code will execute, the memory space of the program will remain the same. Since it's often useful to do an "update" to the same version of a program for debugging purposes, many of our Makefiles contain a rule similar to the following:

	program-update.so: program.so
		cp program.so program-update.so

This is usually called a "v0-v0" update. 

Exercise 1.1
------------

Build the two versions of the example program given in exercise 1.1. Update between them using the tools discussed above.
