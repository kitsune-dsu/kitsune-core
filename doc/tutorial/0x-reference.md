xfgen language reference
------------------------

# Target specification

xfgen allows programmers to write rules at whatever level of abstraction they see fit, for variables, types, or even fields within types. Given the following C code:

	struct foo {
		int a;
		char *b;
	} instance;
	
The following are all valid xfgen specifications:

	struct foo -> struct foo

* to transform all instances of struct foo

	struct foo.a -> struct foo.a
	
* to transform all instances of *field a* of struct foo

	instance -> instance
	
* to transform the variable 'instance'

# Transformation code

Generally, transformation code is written in C. However, xfgen has several extensions to simplify transformation code.

To start, consider:

	$in
	$out
	
These are the implicit arguments of any transformation specification. They have the same type as their specification (they are a `struct foo` in a rule for `instance -> instance`, for example). Any operation that can be performed on a C variable can be performed on `$in` or `$out`.


	$base

This is a third implicit argument, of transformation specifications **for struct fields**. `$base` evaluates to the struct of which that field is a member. For example:

	struct foo.a -> struct foo.a: {
		$out = strlen($base.b);
	}
	
This sets the field `a` in all instances of `struct foo` in the new version to the length of the string in field `b`. 

There is also:

	$xform(oldspec, newspec)


bleh
-----

	void * kitsune_lookup_key_new(char *key)
	void * kitsune_lookup_key_old(char *key)
	char * kitsune_lookup_addr_new(void *adr)
	char * kitsune_lookup_addr_old(void *adr)
	
In many cases, `$in` will be insufficient input data for a successful transformation. These functions allow an xfgen user to request arbitrary state from the old or new versions. The `key` used in the `lookup_key` family of functions is of the same format used in transformer specifications. These functions query Kitsune's internal symbol table, and as such can return static or local variables that would otherwise be inaccessible with, for example `dlsym`.

Kitsune's symbol table also provides a reverse mapping in the `lookup_addr` functions. These functions will return a key suitable for use with `lookup_key` when given an arbitrary address (in Kitsune's symbol table). This is particularly useful when 
