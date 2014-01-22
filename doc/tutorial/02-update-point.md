Thinking about DSU: Update Points
==========================================

A recurring theme of Kitsune's design is that all aspects of a dynamic update are explicit to the programmer. When your program updates, you should have a full understanding of what is occurring (to the boundaries of Kitsune's abstractions).

The first of these concepts we will discuss is the update point.

Placing update points
---------------------

An update point is a simple concept. This is the line in your program that contains the call to `kitsune_update("name")`.

Update points in Kitsune are named so that you can have more than one of them. For example, if your program looks like this example:

      void handle_client(...) {
      	while (true) {
      		switch (input) {
      			...
      		}
      	}
      }
      int main(...) {
      	while (true) {
      		client = get_client();
      		handle_client(client);
      	}
      }

You might want to have an update point for both the loop where the server accepts a new client, and the loop where a client is interacting with the server.

There may be additional places where it might be reasonable to insert an update point. This depends on the degree to which you want your program to be able to update quickly. If you're maintaining the above program, and you only place an update point in the function `main`, then you won't be able to update if a client is engaging with the server. If you place an update point in both `main` and `handle_client`, you'll have to selectively return control to one of the two update points, depending on where you came from.

Exercises
=========

This section's exercise introduces a simple C program that we'll be using to demonstrate dynamic updating with Kitsune, keyvalueserver.c

Exercise 2.1
------------

For your first exercise, read over the program and contemplate the flow of control. Identify the main loop. Place an update point at a reasonable place in the main loop.


Returning to a specific update point
------------------------------------

Let's return to the above program, now instrumented with two update points.

      void handle_client(void * client) {
      	while (true) {
			kitsune_update("handle_client");
      		switch (input(client)) {
      			...
      		}
      	}
      }
      int main(...) {
      	while (true) {
			kitsune_update("main");
      		client = get_client();
      		handle_client(client);
      	}
      }

Upon an update, Kitsune's runtime will invoke main again. It's your job as the developer to ensure that control returns to the correct point in the program. This is why update points are named.

When returning control to an update point, two Kitsune library functions will be convenient to use:

      kitsune_is_updating(void)
	  kitsune_is_updating_from(char *)

`kitsune_is_updating` returns true if your program is in between a successful Kitsune update and the return of control to that update point. For example, in the above program, `kitsune_is_updating` would return true on the first line in main, and on the first line after the declaration of the while loop, but not after the call to `kitsune_update` in `get_client`.

`kitsune_is_updating_from` is identical to `kitsune_is_updating`, except it only returns true if the string passed into it is the same string used to name the update point that Kitsune came from. 

You can use `kitsune_is_updating_from` to selectively return to one of the two update points in the above example program.

      void handle_client(void * client) E_NOTELOCALS {
		MIGRATE_LOCAL(client);
      	while (true) {
			kitsune_update("handle_client");
      		switch (input(client)) {
      			...
      		}
      	}
      }
      int main(...) {
		if (kitsune_is_updating_from("handle_client"))
			handle_client(NULL);
      	while (true) {
			kitsune_update("main");
      		client = get_client();
      		handle_client(client);
      	}
      }

The above example adds or changes four lines:

1. First, in main, we check if the program updated from `handle_client`.
2. If the program did, we call handle_client.
3. Finally, in `handle_client`, we migrate the local variable `client`, which we can do because we
4. Added the Kitsune attribute E_NOTELOCALS to the function handle_client. You'll see more of this later in the next chapter, Migrating State.


Exercise 2.2
------------

Place and name an update point in the client loop.

