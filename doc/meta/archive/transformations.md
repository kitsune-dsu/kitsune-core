

Transformation Effort
=====================

We'd like to build a tool to aid developers in writing transformation
code for Kitsune since we believe that other updating systems are
currently able to provide a better API.

The Ginseng and JVolve (list others) systems provide an effective
interface for writing patches by having developers implement
transformation on a per-type basis.  This means that the work to write
transformation depends directly on the number of changed types in the
program.

On the other hand, the current implementation of Kitsune requires
developers to perform more work.  For instance, if the following data
structure change occurs:

Original:

struct bar {
   int a;
} 

struct foo {
   struct bar *y;
   int z;
   ...
}

struct foo g;

Modified:

struct bar {
   int a;
   int b; // <-- added
} 

For the developer to transform the object pointed to by g, she needs
to copy all of the elements of the old representation of struct foo,
even though struct foo itself didn't change.

JVolve avoids this problem by using opaque pointers that allow an
object to be moved in memory.  If the new representation of bar is
bigger, it can be reallocated and its existing opaque pointer can be
adjusted, without affecting data structures containing references to
the changed object.  Alternatively, Ginseng avoids this effort by
compiling in extra "slop" for all user-defined types.  As long as the
type modifications fit within the slop, they can be supported while
maintaining the validity of exisiting pointers.

The JVolve approach piggybacks on the indirection provided by the Java
runtime environment to simplify transformation.  The C language does
not provide this indirection so these techniques do not apply directly
to Kitsune.  Likewise, although adding slop to types (as Ginseng does)
simplifies transformation, it can increase memory usage, hurt
performance, and reduce evolution flexibility.  (This approach uses
special compilation to insert the slop.  Ginseng also uses special
compilation to maintain version information for types allowing them to
be transformed as needed (i.e. lazily).)

Our Proposed Approach
=====================

We seek to provide a similarly elegant transformation API for Kitsune
by using a compiler to generate the tedious portions of transformation
code.

For the example above, the developer could write a mapping between the
old and new representation of bar.  (Even better, the developer could
just specify how the added field "b" on bar should be initialized.)
From this information, the compiler would generate code to traverse
all types in the program performing the necessary copying,
transformation, and deallocation.

Transformation API
------------------

This is the simplest version of transformation.  Here, we define how
the old representation of bar should be transformed into the new
representation.  This corresponds closely with type transformations.
This form of transformation requires the developer to indicate how
each field of the new type should be initialized from the old type.

#define KITSUNE_XFORM("bar_old -> bar", xform_bar)
void xform_bar(bar_old *old, bar *new) 
{  
  new->a = old->a;
  new->b = 0;
}

There will often be times when the developer chooses to only implement
some of the transformation for fields within a type and will opt for
the automatically generated transformation for the rest of the fields.
For example, imagine that the "a" field in the previous example is
more complex than an integer.  In such cases the developer can call
into the transformation framework using "kitsune_xform_auto".  In the
example below, kitsune will use rules for transforming from the old
type of a to the new type.

#define KITSUNE_XFORM("bar_old -> bar", xform_bar)
void xform_bar(bar_old *old, bar *new) 
{  
  kitsune_xform_auto(&old->a, &new->a);
  new->b = 0;
}

Initialize a single added field.  By default the other common fields
will be copied.  The idea is that, if only one or two fields on a
struct are added or changed, most of the transformation code will be
trivial and we can generate it.  The developer can choose to write
transformation only for the fields that need it.  If a field is added,
but the developer forgets to write transformation, we can issue an
error (or a warning).

#define KITSUNE_XFORM("bar_old -> bar [b]", xform_bar)
void xform_bar(bar_old *old, int *b)
{
  (*b) = 0; // init added field
}

We may need a mix of type-based and general transformation.  For
example, if a list object pointed to by a particular struct is changed
so that it is kept sorted in the new version of the program, we
probably can't just write transformation to sort all lists in the
program.  To handle this case, we allow paths.

#define KITSUNE_XFORM("baz.data -> baz.data", xform_baz)
void xform_bar(list *old_data, list *new_data)
{
   kitsune_xform_auto(&old_data, &new_data);   
   list_sort(new_data);
}

Transform a global variable.

#define KITSUNE_XFORM(".h_old -> .h", xform_bar)
void xform_bar(int *h_old, int *h)
{
  (*h) = (*h_old) - 10;
}

Depend on multiple inputs.  In this case, in order to transform from
the old to the new representation of "bar", we also look at one of the
fields on a global variable called "config".  The benefits of this are
using our handy "path" syntax to grab needed values, and allowing us
to know when certain pieces of data can be deallocated.

#define KITSUNE_XFORM("bar_old, .config.foo_var -> bar", xform_bar)
void xform_bar(bar_old *bold, foo *foo_var, bar_new *new)
{
  (*h) = (*h_old) - 10;
}

Initialize an added global variable.  Seems less useful.  Maybe drop
this form!

#define KITSUNE_XFORM(".h", xform_h)
void xform_h(int *h)
{
  (*h) = 42;
}

Comments:

It will be annoying for the developer to copy all of the old and new
versions of changed data types into the file containing the
transformation code.  This problem is exacerbated by the need to give
new names to these data structures when there is a name conflict.

Some options:

1. Access fields off of old data structures using a special syntax
   like FIELD(ptr, "name") that is translated to calculated accesses
   based on some "version data" that we pass forward.  This seems like
   it would suck for the developer.

2. Build a compiler to rewrite transformers from normal C to (1). 

3. Have a preprocessing compiler that looks at the transformation
   rules and sticks in the needed, renamed type definitions.  If we
   went this route, we could even choose to make the syntax nicer!
   For example, we could eliminate the need to give the transformation
   functions names and also drop the need to declare types for
   arguments and local variables.  We could provide a macro OLD(symb)
   which would provide the name of the old type 


Data Structure Annotations
--------------------------

Have some global definition indicating the default malloc and free
functions.

TypeAnnot ::=
   ALLOC(allocator, deallocator)    <- to specify malloc/free type functions
   INIT(initializer, deinitializer) <- init for data types, takes no arguments
   PTR
   PTR_ARRAY(NumElements)
   ARRAY(NumElements)
   GEN_VAR(t)                       <- generic type identifier
   GEN_USE(t=CType, ...)            <- asignment of generic type

NumElements ::= 
   N                    <- Integer Size
   self.elem            <- Field of integral type indicating size

CType ::=
   int | long | float | double | ...    <- primitive types
   struct_name                          <- struct types
   name                                 <- typedefs
   *CType                               <- ptr to CType


Examples:

typedef void E_T(t) ** generic_ptr_ptr E_GENERIC(t);
generic_ptr_ptr E_T(t=*int) use;

struct foo {
  int x;
  struct foo * E_PTRARRAY(12) y;
  struct foo * E_PTR z;
  struct foo * E_CUSTOM(malloc, free) q;
  struct foo * next;
};

struct list {
  void E_T(t) *data;
  struct list E_T(t=<t>) *next;
} E_GENERIC(t);

typedef struct list_t E_T(t=<struct foo>) foo_list;

struct list2 {
  int length;
  long data[E_ARRAY(self.length)];
};

typedef struct list_t E_T(t=<p>) list E_GENERIC(p);



Implementation
--------------

During the traversal, each time a pointer is encountered check to make
sure it is either NULL, a pointer to something on the heap, or there
is a transformation rule for that pointer (that will presumably update
it correctly).  This is to ensure that stack/global/function/etc ptrs
do not leak into the heap state of the new version where they will
become invalid after old-version code is unloaded.

Also, during the transformation, a mapping is kept between the old and
new addresses for each of the objects transformed.  If we can
determine, at some point, that the old version of a type is no longer
needed for future transformations we can free it.