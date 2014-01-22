# Debugging Kitsune programs with GDB

## Advanced Breaking

### Python-powered GDB

In GDB 7.3, we'll be able to write python that listens for a
breakpoint to trigger, and when it does, refreshes our other
breakpoints. This doesn't work on the GDB currently in Ubuntu. 

### Advanced Breaking

You can use the `rbreak` GDB command to break on every symbol that
matches a given regular expression. For example, to break on every
definition of `do_main_loop`, you would use the following command:

`rbreak do_main_loop`

This will also catch some debugging symbols. In the future, we'll
provide a GDB command (implemented in Python) to break on all versions
of a function. 

## Common Errors

### "Generic Threading Error"

This error occurs when execution transfers into the kitsune target from
the driver program. After that, no gdb commands will work, though the
gdb still reports that the program is executing.

This is caused by GDB being unprepared for libpthreads being
introduced into the program image -- gdb needs advance warning of this
so it can configure its thread management.

To solve this, include libpthreads.so in your LD_PRELOAD environment
variable on the GDB command line. For example:

`LD_PRELOAD=/lib/i386-linux-gnu/libpthread.so.0 gdb driver`

## Helpful GDB Commands

### x/i <address>

This will tell you which function a given code-segment address corresponds to. Very useful when debugging function pointer errors.

