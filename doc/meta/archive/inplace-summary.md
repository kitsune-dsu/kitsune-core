
In-place Ekiden
===============

Following is a summary of the current in-place updating mechanism that
we have implemented for Ekiden.  The current implementation is quite
simple, but is lacking some critical features.

Files
-----

Literate programming-like annotation of the current, simple
implementation of in-place updating:

- [driver.c](driver.html)
- [ekip.c](ekip.html)

Missing Features (compared to normal Ekiden)
--------------------------------------------

1. The current implementation only supports accessing old-version
   state through global variables.

2. In this code, there is nothing to ensure that the first attempt to
   transform a piece of state will be memoized so subsequent attempts
   will return the same value.

Warts (smaller concerns to keep an eye on)
------------------------------------------

### Performance

The choice to compile as a shared library may introduce some
performance overhead.  For example, it requires building with position
independent code (-fPIC) which does introduce a bit of overhead: it
disables IP relative addressing for global accesses and jumps, and
instead requires lookups to the global offset table.

This may not be so bad on 32-bit architectures since they don't
support IP relative addresses anyway, but it will have some effect on
64-bit machines.  There may be other performance effects due to
building as a library.  I think we need to quantify these since low
steady-state overhead is one of our key selling points.

### Function Pointers

The current implementation (and redis patches that use it) funnels
some function pointers through void pointers which is not valid C.
Obviously it works OK on gcc, but it does present a compatibility
concern.


Big Challenges to Address
-------------------------

### State Transformation

We need to make the process of writing transformation code (and code
to free old version heap state) easier.  The current plan is to adjust
the serialization generator to support these things.

State passed forward from the previous version may contain non-heap
pointers (e.g., to global variables, functions pointers, or to stack
variables).  None of these will continue to make sense at the new
version, so we need some way of fixing the pointers:

- Stack based data (not handled at the moment): probably need to
  memcpy the data itself (which requires noting the sizes of tagged
  state) or the entire stack so we can access it following the
  longjmp.  If the stack isn't too huge (empirical question), perhaps
  the easiest solution is to just memcpy the whole stack and require
  accesses to old-version stack pointers by the new program to go
  through an offset adjustment function/macro.

- Manually adjusted: the users manually writes code to adjust these
  pointers to point at the right places (current approach)
- Automatically adjusted: our transformation framework can take care
  of generating code to traverse the data replacing these pointers
  with their new counterparts.  Update-time performance, even for a
  v0->v0 update may suffer here (but is no worse than the manual
  case).
- Through compilation/loader: we could attempt to ensure that the new
  versions of globals are placed at the same addresses in memory as
  the old versions.  This would mean that pointers would remain valid
  if the function persists across versions.  Users could specify
  mappings of old->new globals and we could place functions at the
  right place in memory corresponding to the mapping.  (Note: this
  would also require copying all the old global variable values
  elsewhere in the heap so we could continue to access their values.
  This feels brittle, but should improve update-time performance. 
- Create indirected versions of all globals / function pointers and
  ensure that all pointers in the heap point to the indirected version
  rather than the globals themselves.  Then transformation just needs
  to make sure that all these indirected versions get updated.  (If we
  could tolerate a slightly heavier analysis, we could do this only
  for globals whose address is taken.)

### Multithreaded

Need to dig into the multithreaded Ginseng examples to figure out what
might work here.  Current thinking is that creating versions of
blocking system calls that unblock when an update has been requested
(e.g., returning EINTR or "EUPD") might do the trick.

<!-- LocalWords:  Ekiden LocalWords EINTR EUPD globals memoized -->
