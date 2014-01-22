vsftpd Experiences
==================

Observations
------------

We've chosen to add 5 update points.  These update points correspond
to:

1. the main connection acceptance loop (standalone.c)
2. the client request processing loop (postlogin.c)
3. the privilege-separation process loop (postprivparent.c)
4. the user login attempt loop (prelogin.c)
5. the login-checking loop (twoprocess.c)

>>Explain why we chose to include all these update points.  How would
program updatability by impacted by removing any of them?

These update points were chosen with two goals in mind. First, we wanted updates to be initiated quickly once requested. To meet this requirement all long lasting loops (1, 2, 3) were modified with an update point. Loops 4 and 5 only exist until the user has managed to log-in. Second, ease of reasoning...
/// (still need to figure out why vsftpd doesn't update fine without 4 and 5; I'll try to figure that out by friday and update this paragraph.)

>>There are signals whose handler needs to be updated.  How was this
done?

Vsftpd has a function for setting signal handlers. For example updating the sigchld handler looks like this:
vsf_sysutil_install_async_sighandler(kVSFSysUtilSigCHLD, 
  twoproc_handle_sigchld);

>>There were only a small number of state items to be transferred.  This
is largely due to vsf_session holding a bunch of these state.  Also,
the configuration options are not passed forward, and are reread from
the configuration file instead.  What would be the impact of
transferring those state items on programmer effort?

Transferring these states would be borderline trivial since they all exist in tunables.c with no type changes and minimal name changes. However, it seems more desirable to reinitilize the configuration options rather than to transfer them since that allows new options to be enacted without stopping vsftpd. An interesting use of updating, actually, could be to do vX -> vX updates (same version) with changes done to the configuration file only. Otherwise, if it was desired to change the current vsftpd configuration, the server would have to be restarded with the new conf file.

>>We had to remove static modifiers, although we should be able to undo
this change now that we have compilation tools to make statics
available to the new version.

Will do.

>> What else did we learn?

Several things:
-chroot, which was used as a security feature, can mess up update pathing.
-Deeply nested blocking loops waiting for input can delay updates indefinitely. This can be avoided by having an update point within the loop (this requires passing in the update point's name). This in effect creates an update point with an already used name. In my experience this was not too difficult to reason about, but it could create problems if care was not taken to pass in the correct update name to the loop.
-Some versions do not even require any custom update functions for state transfer (for example, 2.0.1->2.0.2).
-Most of the update work for vsFTPd ended up being program flow control during an update rather than state tranfer.
-Two functions were required to be made visible (i.e. static removed and function prototype added to header file) to help with control flow. Both changes were avoidable by using more program flow redirection.
