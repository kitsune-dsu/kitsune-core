Data Migration
--------------

If you want more manual control, or aren't using the Kitsune compiler

	NOTE_LOCAL /* macro */
	NOTE_LOCAL_STATIC
	MIGRATE_LOCAL_STATIC
	NOTE_AND_MIGRATE_LOCAL
	NOTE_AND_MIGRATE_LOCAL_STATIC
	
The behavior of the `NOTE_`, `MIGRATE_`, and `NOTE_AND_MIGRATE` macros is the same as 
