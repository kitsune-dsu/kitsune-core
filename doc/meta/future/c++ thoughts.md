
Thoughts on Supporting C++ with Kitsune
======================================

This file should be used to note misc. concerns and ideas about how we
might eventually support C++ with Kitsune.

Handling C++ Code
-----------------

The transformation generator has been written to use CIL as a
front-end, but not for type comparison or transformation code
generation.  This leaves the possibility of switching to Clang (LLVM)
and gaining the ability to support either C or C++.

Destructors
-----------

One concern is that we might not be able to delete the old versions of
objects if they have destructors that might go muck with other state
or release other resources.

One idea is that, if we're using a source-to-source compilation
strategy, we could rewrite the destructor to execute conditionally
depending on whether we're in the midst of an update.

When I chatted with Charlie (Emery's student @UMass) about C++
support, he mentioned the possiblility of just leaking objects instead
of freeing them and relying on them being pushed off to secondary
storage under memory pressure.

Constructors
------------

There may be tricky cases with constructors (e.g., there isn't a
default constructor with which we can create a new instance of a
desired object).  There may also be interface enforced policies (like
singleton objects).  Perhaps all these problems can be fixed with
custom compilation that lets us create objects in the way that we
want.  Details are still sketchy here.

