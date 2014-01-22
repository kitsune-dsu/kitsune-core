Data Migration
==============

Dynamic software updating is useful because it preserves the valuable state of a program while upgrading whatever code is executing. As such, data migration is the heart of DSU. Kitsune makes sensible choices for the default behavior of data migration whenever possible, while attempting to remain configurable enough to be flexible.

Auto-Migration
--------------

    void kitsune_do_automigrate(void)
	E_MANUAL_MIGRATE /* annotation */


In Kitsune, every global variable and local static variable is tracked and migrated forwards by default. When this happens in program execution is left to the discretion of the developer, who will use the API function `kitsune_do_automigrate`.

After `kitsune_do_automigrate` returns, all global and static local variables will contain the value they had upon an update. This is a sensible default for most programs. Not changing the control flow of initialization code is usually a cleaner alternative to using annotations to disable automigration for variables which should be reset upon update.

It's also possible to perform data migration completely manually (though this usually produces repetitive and error-prone code), by passing the Kitsune `ktcc` compiler the `--dontglobalreg` option, which disables variable tracking and automigration.

Manual Migration
----------------

    MIGRATE_GLOBAL(var) /* macro */
	NOTE_STATIC /* macro */
	MIGRATE_STATIC /* macro */
	NOTE_AND_MIGRATE_STATIC /* macro */
	
If you wish to manually migrate the program state rather than have Kitsune auto-migrate it, every variable to be migrated must be passed to the `MIGRATE_GLOBAL` macro. This macro will perform what `kitsune_do_automigrate` would have done for that variable, and will return true if the variable has been migrated. 

It's safe to call `MIGRATE_GLOBAL` when an update isn't occurring -- doing so will cause it to return false, which allows a convenient idiom when manually migrating state:

    if (!MIGRATE_GLOBAL(var))
		var = initial_value;
		
To manually update a static variable, it's necessary to register the variable with the Kitsune runtime with the `NOTE_STATIC` macro. Since this is often done at the same time migration would be done at an update (with `MIGRATE_STATIC`), Kitsune provides a convenience macro, `NOTE_AND_MIGRATE_STATIC`. This macro has the same behavior as `MIGRATE_GLOBAL`.

Migrating Local Variables
-------------------------

Kitsune is unable to register and track local variables, because all registration and tracking occurs before the program's `main` is called, and thus, before any local variables exist. 

As such, Kitsune does **not** auto-migrate local variables. Any migration of local variables must be **manually instrumented** by the developer. Fortunately, migrating local variables is typically a rare occurrence.

Kitsune's handling of local variables is similar to its handling of static variables. Kitsune provides the following macros:

	MIGRATE_LOCAL
	E_NOTELOCALS /* annotation */ 
	
To mark a function as having updatable local variables, apply the `E_NOTELOCALS` annotation to it as such:

    int main(void) E_NOTELOCALS {}

Within the body of that function, you can now use `MIGRATE_LOCAL` to migrate local variables. `MIGRATE_LOCAL` has the same behavior as `MIGRATE_GLOBAL` -- when it returns, the variable will have the value it did at update time, and the macro will return true if the variable was migrated, and false if an update isn't occurring.

Conclusion
----------

This chapter introduced data migration, the process of bringing forward state from the old version into the new process after an update.

However, the representation of state often changes between versions. The migration techniques discussed in this chapter do not address the process of *state transformation* -- altering the previous version's state to meet the expectations of the new version.

State transformation is often the most challenging part of writing a software update, and its discussion will comprise the bulk of the remainder of this tutorial.

Exercise 4.1
------------

For now, retrofit keyvalueserver.c to migrate all important state. Which state is important to migrate? What Kitsune features do you have to use to migrate it?
