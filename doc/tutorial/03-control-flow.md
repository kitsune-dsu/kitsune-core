Control Migration
=================

Now that you've placed an update point, you've introduced a path through your program that can be taken either after an update, or after starting from scratch. The intended behavior of your program will have to change to accommodate this.

For example, consider this excerpt from our key-value server:

    int
    main(int argc, char **argv)
    {
      ...
      init_store();
      server_sock = init_listen_socket(5000);
	  client_sock = 0;
      ...
    }

If main is executing for the first time, these two statements will properly initialize the key-value store and get a socket for the server to listen on. If main is executing after an update, this behavior will be incorrect -- we'll want to use the socket and store from the previous version rather than creating a new one from scratch.

Kitsune contains two functions in its API that allows you to differentiate between the first execution of your program, and execution after an update.

    int kitsune_is_updating(void)
	int kitsune_is_updating_from(char * update_point_name)
	
The first, `kitsune_is_updating`, takes no parameters and returns true if the program is been dynamically updating, much as you might expect.

The second, `kitsune_is_updating_from`, allows you to distinguish between updates from different update points. It takes the name of an update point as a parameter. 

Both `kitsune_is_updating` and `kitsune_is_updating_from` only return true from the point where the new version of `main` is invoked to the first update point. At that point, the old version's shared object is unloaded, and the program is considered by Kitsune to have returned to normal execution.

Consider these functions used to adapt the above snippet:

    int
    main(int argc, char **argv)
    {
      ...
	  if (!kitsune_is_updating()) {
		  init_store();
		  server_sock = init_listen_socket(5000);
	  }
	  if (!kitsune_is_updating_from("client_loop"))
		  client_sock = 0;
      ...
    }

In the first conditional, we will re-initialize the socket and store only if we are not updating. In the second conditional, we only care about not re-initializing the client socket if we've updated from the client loop.

These API calls allow you to reason about paths through your program involving an update explicitly. You should consider a call to `kitsune_update` equivalent to a call to `main` for when reasoning about control flow. After that call, calls to kitsune_is_updating will be true until the update point is reached again, at which point your program should be in the same state as it was before the update -- just at the new version.

Exercise 3.1
-------------

Modify keyvalueserver.c to avoid re-initializing key state after an update.

When you complete this exercise, you might notice that as it stands, your modified keyvalueserver.c is of little use, and if it's run and updated, it will crash. 

This is because we've changed the program such that control flow is properly migrated back to an update point after an update, but we haven't yet introduced *data migration*. Data migration is our term for code that instructs Kitsune to bring forward the old version's state. The [next section](04-data-migration.html) will introduce this concept more fully, and cover the relevant parts of the Kitsune API. 

