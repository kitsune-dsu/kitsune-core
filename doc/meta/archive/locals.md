
Summary of Local Variable Handling
==================================

Current Implementation
----------------------

=== Summary ===

Currently, we have two ways of making local variables accessible to
the next version:

1. manual annotation, and

2. the "stackvars" module of the kitsune compiler (this has the
   downside of introducing overhead everywhere since it doesn't use a
   call-graph analysis to limit where it tracks locals.

A third option would be to add a whole-program, call-graph analysis to
the compiler to reduce runtime overhead.

=== Manual Annotation ===

Manual annotation requires:

1. adding ENTER_FUNC(fun) at function entry for any functions with local
   variables we want to make available to the next version.

2. adding NOTE_LOCAL(var) to make a local variable accessible to the
   next version, MIGRATE_LOCAL(var) to attempt to initialize a
   variable from state transferred from the previous version.  (If
   both behaviors are desired, the developer should employ
   NOTE_AND_MIGRATE_LOCAL(var).)

3. adding LEAVE_FUNC(fun) prior to each return call in the function.

The main downsides of this technique are that we may forget to
annotate certain variables and it may be alot of work to add these
calls throughout the code.  Yudi has some experience adding these
annotations manually for libevent and found that it wasn't too much
work (although libevent may not be typical).

Note that these calls may produce some unavoidable steady-state
overhead for the same reason shown for the automatic compilation in
the next section.

=== Automatic Annotation ===

The kitsune compiler will automatically instrument the code to note all
local variables when the "--dostackvars" flag is enabled.  This will
insert code to record every function entry and exit and all local
variables for each function.

This has the benefit that we never forget to make a global variable
available to the next version.  It has the downside that noting some
unneeded locals may introduce overhead.  If this overhead only impacts
startup, then maybe that's a price we're willing to pay.

Because "stackvars" currently notes locals in *every* function, it
will introduce steady-state overhead throughout execution. We could
implement a static analysis of the call-graph to determine which
functions may be on the stack when an update point is reached.  This
will be conservative of course and may be too conservative. (I'm
pretty confident that the analysis would do a poor job with OpenSSH,
for example, because one of the main loops is reached through a
dispatch table of function pointers.)  Perhaps such an analysis would
work great for most programs, however.  If the update point is in
library code (e.g., libevent), then the compiler would need to know
which library functions may reach an update point.

Currently, we don't have need for any whole-program analysis.  I'd
really like to avoid adding it, but perhaps it's unavoidable?

Even if our analysis were precise, we might still introduce steady
state overhead.  For example if a program is structured as follows

loop() {
  while (1) {
     kitsune_update("client_loop")
     bar();
  }
}

intermediate() {
  // a bunch of locals
  loop()
}

main() {
  while (1) {
     kitsune_update("main_loop")
     intermediate();
  }
}

we'd always need to note the locals of "intermediate" which is hit
during iterations of the main loop.  Perhaps this isn't such a big
deal, performance-wise for most programs, although it could be bad
with a conservative analysis (e.g., with OpenSSH).

Accessing Local Variable State
------------------------------

Currently, we have an overly simplistic way of accessing local state:
based on only the containing function name and the variable name.
This assumes that each function is on the stack just once (this is
true so far in our examples, but perhaps we'll find cases where it is
not).  Should we put time into thinking about how the API should look
for those cases?

Also, there is some possibility that patch writers may eventually want
to examine the stack of function invocations in their transformers.  I
think we're conjecturing that this won't be needed often, but members
of the audience at HotSwup were skeptical, so perhaps we should
consider exposing this information.
