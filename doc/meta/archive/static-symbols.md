
Support for Static Symbols
==========================

Problem
-------

When a program is updated, we need to be able to retreive the
addresses of static symbols from the prior version during
transformation for two potential reasons:

1. To access the values of variables in the old version in order to
   assign appropriate values to new version variables.

2. To update pointers (to static variables or functions) in the heap
   to the appropriate variable/function in the new version.

However, we cannot look up the addresses of static symbols using the
shared library infrastructure that we use for other symbols.

Possible Solutions
------------------

=== Remove static modifier === 

This can be done manually or automatically.  Automatic removal will
(in general) require some renaming of symbols, possibly based on the
filename (e.g. __kitsune_foo_c__bar, for the symbol bar in foo.c).

An issue that Yudi has pointed out is that this may be undesirable for
library code since it will polute the global namespace with a bunch of
extra symbols, possibly colliding with application code.

=== Use a compiler to introduce our own per-file symbol tables ===

We can generate an array with a carefully constructed name (like:
__kitsune_foo_c__symbols for the symbols in the file foo.c).  We could
decide whether this will include only the static symbols or all
symbols.  If it includes only the static symbols, we'd have separate
interfaces for accessing a static (using the filename and symbol name)
or a non-static (using just the symbol name).

The symbol table itself would be exported and would collide with
symbol tables for other files with the same name.  Perhaps there is
some systematic naming scheme to help avoid this.

=== "Register" static variables / functions ===

If we could find a way to have certain functions called automatically
when a shared library is loaded, we could generate code to register
each static symbol in a table maintained within Kitsune.

GCC has the "constructor" attribute that can be applied to functions
causing them to be called "before main".  I think that when a shared
library contains "constructor" functions, those functions will be
called when the shared library loads.  If so, we could generate
constructor functions that register all static symbols with our
library.

I think this might be the cleanest choice of those presented in this
file.

=== Manually annotate statics for transfer ===

Yudi has noted that the way we currently note local variables within a
function for transferring (i.e., popping their name, address, and size
onto a stack for the lifetime of the function invocation) can also
accomodate addresses of static global/local variables.  Only the
particular static variables noted in the previous version would be
available in the next version.

This may not address the problem of making the addresses of static
functions/globals available to update addresses in the heap (unless we
write similar annotations for functions).

