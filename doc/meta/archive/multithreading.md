
Multithreaded Kitsune
====================

Simultaneous Updating
---------------------

We had previously discussed the idea of updating all threads
simultaneously.  To do this, we would prevent threads that are waiting
for IO from blocking, since blocking might prevent other threads
waiting at update points from making progress in a reasonable amount
of time.  We hypothesize that this will allow updates for many
programs to proceed in a timely manner.

This approach could fail when a program uses a large number of threads
or when update points are not reached frequently for some threads.

Allowing Multiversion Concurrent Execution
------------------------------------------

Another possibility is to allow each thread to update separately as it
reaches an update point.  We can support this in our in-place system
because we still have all of the code and variables from each version
available until we unload the previous version.

The main challenge with this approach is ensuring that shared state is
valid and accessible for both the old and new code that is running.

### Exploiting Mutexes

A trick for ensuring consistent access to variables representing
shared state would be to ensure that the old representation is valid
whenever a thread at the old version acquires the lock that protects
it (and likewise for new-version state/threads).

This would require: 

1. having the developer specify forward and backward state
   transformation for shared state items (lenses would be an fancy
   implementation technique for these, but probably not one that we
   would explore right away).

2. having the developer provide a mapping between mutexes and the
   shared variables they protect in the program, probably through
   annotation.

We could then piggy-back on pthreads lock operations to run the
correct forwards/backwards transformations for the shared state
protected by a lock whenever a thread at a particular version acquires
it.

### Potential Problems

1. Other synchronization mechanisms (reader-writer locks, semaphores,
   shared-state without synchronization) are less straightforward to
   support.

2. It is possible that the granularity of code for which the developer
   provided synchronization is sufficient to ensure correct execution
   at a single version, but is insufficient for DSU.  (e.g., perhaps
   accesses to two shared state items need to be atomic w.r.t. code
   versioning for a particular patch, but not atomic in terms of
   concurrency).

3. Perhaps it's not possible to implement bi-directional transformers
   for certain kinds of changes.

4. How to support fine-grained locking?  How to register the
   association between fine-grained locks and non-global data (like a
   single element of a list)?  Perhaps manual code changes could be
   required in these specialized cases.  Also how to support
   forward/backward transformation for a piece of state that is just
   one part of a larger data structure?  Can we avoid transforming the
   whole structure?  If not, we'll need to acquire a coarse lock
   (since other threads might access other parts) and possibly reduce
   concurrency drastically.

5. Slowdown due to expensive transformation when a lock is acquired.

