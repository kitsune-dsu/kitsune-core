
Gotchas Encountered when Applying Kitsune
========================================

This document provides a place to record solutions to tricky problems
that you encounter with *using* kitsune.

driver can't find init symbol
-----------------------------

If you run into this error:

 driver: driver.c:168: main: Assertion `init_func != ((void *)0)' failed.

when you've definitely included the kitsune library when linking your
shared library, the problem might be that you have no code in your
program that forces the kitsune library to actually be included.  This
would be unlikely for a completed program that supports updating, but
could happen during early development or for certain test cases.

The solutions are to either:
1. make use of one of the kitsune functions in the code of the program
2. add "-u kitsune_init_inplace" to your gcc arguments when linking,
which will force the kitsune library to be included to make the
kitsune_init_inplace symbol available.