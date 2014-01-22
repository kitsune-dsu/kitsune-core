
Memcached Experience
====================

I confess I'm not as up to speed on what we've learned from memcached
so far, so I don't have much to contribute here.

We had previously chosen to use a version of libevent prepared for
updating, but we're now considering switching to the other approach of
just resetting libevent's state (as we have done in Tor).  Why are we
considering the switch?

Memcached had a bunch of static variables that are now handled using
our cil compiler rather than by removing the static modifier.  Has
this change solved the static-related problems we were having before?

Memcached uses threads, but (to my knowledge) we haven't starting
addressing that problem yet.

What is the overall status of memcached?

What other challenges have we encountered?

What are the impediments to v0->v0 updates and/or v0->v1 updates?