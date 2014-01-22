
Using the Kitsune Compiler
=========================

There are several functions performed by the Kitsune compiler to make
constructing updates easier:

1. stackvars - automatically register all local variables with our
   library so they will be accessible when we update.

2. [generate transformers - COMING SOON]

3. [register statics with runtime - COMING SOON]


Building the compiler
---------------------

The compiler is located in tools/ekcompiler in the source repository.
To build it, you need to run:

  [in the cil-1.3.7 folder]
  ./configure 
  make


Running the compiler
--------------------

After building, the compiler can be used by running
  cil-1.3.7/bin/cilly
which should mostly operate as a drop-in replacement for gcc.


Stackvars Details
-----------------

=== running the stackvars module ===

The stackvars module rewrites each function in the program to register
its local variables with the Kitsune runtime system.  This ensures that
any local variables that are active on the stack are accessible to the
next version of the program, whether the developer chose to manually
annotate them or not.

Here is an example use of the compiler to build input_src.c that
invokes the stackvars module:

  $(PATH_TO_CILLY)/cilly input_src.c --dostackvars --keepunused

For debugging / sanity checking purposes, you can add the command line
argument "--save-temps" to keep the rewritten source code around for
your inspection.  If the input file is "input_src.c", the rewritten
version will be "input_src.cil.c".

=== example output ===

Adding stackvars will cause a program to be modified as follows:

Original code:

  void test1(void) {
    int x;  
    test2();
  }

Output code (simplified):

  void test1(void) 
  { 
    int x ;
    stackvars_note_entry("test1");
    stackvars_note_local("x", &x, sizeof(int));
    test2();
    stackvars_note_exit("test1");
    return;
  }

In summary, stackvars_note_entry() records that we have entered the
"test1" function in the runtime system.  For each local variable,
stackvars_note_local() is invoked with a string representation of the
variable's name, its address and its size.  Prior to each "return"
call in the function, stackvars_note_exit() is called to discard the
data for the stack frame from the runtime system.
