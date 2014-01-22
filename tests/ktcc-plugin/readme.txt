KS: This roughly demonstrates how pluginv2 will call the mainv1's kit_reg_var instead of mainv2's kit_reg_var.
The setup is not perfect, but it is reproducing the same behavior I see in Snort.


Build:  make
Warning -  makefile dependencies not perfect, do "make clean; make" when making changes.


Running:
./driver 


*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*
*   Initial Problem 									  *
*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*

Output:
driver: Calling dlopen on ./mainv1.so from driver
mainv1: inside kit_reg_symbols, calling kit_reg_var
**OLD:  inside kit_reg_var:v1, called from mainv1.c
driver: loaded ./mainv1.so.  Jumping into ./mainv1.so's main
mainv1: inside main, loading pluginv1.so from mainv1.so
pluginv1: inside kit_reg_symbols, calling kit_reg_var
**OLD:  inside kit_reg_var:v1, called from pluginv1.c


<-----fake updating---->

driver: Calling dlopen on ./mainv2.so from driver
mainv2: inside kit_reg_symbols, calling kit_reg_var
******NEW:  inside kit_reg_var:v2, called from mainv2.c
driver: loaded ./mainv2.so.  Jumping into ./mainv2.so's main
mainv2: inside main, loading pluginv2.so from mainv2.so
pluginv2: inside kit_reg_symbols, calling kit_reg_var
**OLD:  inside kit_reg_var:v1, called from pluginv2.c   //////////////Problem: this should be "NEW" v2 instead of "OLD" v1
exiting all


*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*
*   Current solution									  *
*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*
http://www.technovelty.org/code/c/symbol-version-deps.html


Basically, it's just tagging the versions in the linker.
*  snort.so would get tagged with the flag "--version-script=snortv0.ld"
*  snortv0.ld is a 3 line file:
VERS_0.0 {
        global: *;
};

Then you do the same thing for snortv1.so, only change the number
(like VERS_1.0).


And then, for pluginv1.so, just add a -L./ -lsnortv1 or whatever
version on the link line.  This should work well because the plugins
are compiled at the same time so linking them with matching version #
shouldn't propose any problems.

I inspected the .so files and it doesn't seem to change anything
(regarding visibility/hidden/default/etc).  The article
 also didn't indicate any other changes to the .so.  There may be something i'm
missing but so far so good.
