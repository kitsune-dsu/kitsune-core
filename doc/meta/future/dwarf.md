
Notes on Using Debug Info for Kitsune
====================================

We want to support updating programs using Kitsune without requiring
that every critical local variable have tagged in anticipation of the
update.  One approach that we considered was using DWARF debugging
information to make this information available at runtime.

I spent a couple days looking at this prospect and summarize my
findings here.  For now, we have determined that these techniques may
not be suffienciently robust and are opting for a source-to-source
compiler approach instead.

Approaches for Grabbing Data off the Stack
------------------------------------------

Ted's Ginseng activeness safety analysis and POLUS use ptrace to
connect to the running code and access registers to reason about the
makeup of the stack.  Ginseng calls "nm" to extract the addresses of
the functions in the executable and compares those with the function
addresses on the stack.

The backtrace and backtrace_symbols functions provided in glibc's
execinfo.h header file compute the bounds of the callstack using the
address of a local variable (for the bottom of the stack) and using
the special variable __libc_stack_end (for the top).  They then step
through the stack looking up the names of functions using the dynamic
loader (i.e., using dladdr() which resolves the name and the file
containing the symbol corresponding to a particular address).  The
execinfo functions use platform-specific routines to step backward
through the stack.

Another option for getting data about a program is LIBBFD (binary file
descriptor library) which allows the developer to extract address and
symbol information from executables/shared libraries on a variety of
platforms.

Summary: None of these approaches actually use DWARF data for their
operation.  Also, the code to traverse the stack is platform specific
and may be unweildy for us to maintain (however, this could perhaps be
mitigated considering only a subset of architectures, such as
i386/x86_64).

Grabbing Dwarf Data
-------------------

LIBDWARF can be used to programmatically extract debug info from
programs.  The commandline tool dwarfdump can be used for manual
inspection.  These tools expose symbol names, relative addresses and
type information for variables.  I have noticed that DWARF also
contains information about which functions are inlined.  Perhaps that
information, combined with debugging line-number information could be
used to reconstruct the stack corresponding to the orginal program
even in the presence of inlining.  I wonder if gdb does any of this?

It should be possible to combine the techniques used in glibc's
execinfo backtrace functionality with DWARF information to lookup
values on the stack by name.

If we decide to reconsider using the DWARF info in the future, it
would be good to examine gdb's code for viewing local variables.
Although gdb uses ptrace, which is (probably) not neccessary for our
needs, their handling of DWARF data and connecting it to addresses on
the stack would directly apply.

Robustness 
----------

I found that some small, simple programs I experimented with produced
different backtraces under all levels of gcc compiler optimization
(these toy programs were initially based on the code on the man page
for backtrace).  This left me quite concerned that this technique
might only be really useful if we disable many compiler optimizations.
I did not yet examine the problem of local variables being allocated
entirely to registers or eliminated entirely, although those are
surely additional threats to robustness.