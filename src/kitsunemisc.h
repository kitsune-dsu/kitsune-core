#ifndef _EKIDEN_MACRO_TRICKS
#define _EKIDEN_MACRO_TRICKS

/* 
   Determine number of variadic arguments to macro:
   http://stackoverflow.com/questions/2632300
*/

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)

/* C99-style: anonymous argument referenced by __VA_ARGS__, empty arg not OK */

#define N_ARGS(...) N_ARGS_HELPER1(__VA_ARGS__, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define N_ARGS_HELPER1(...) N_ARGS_HELPER2(__VA_ARGS__)
#define N_ARGS_HELPER2(x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, n, ...) n

#elif defined(__GNUC__)

/* GCC-style: named argument, empty arg is OK */

#define N_ARGS(args...) N_ARGS_HELPER1(args, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)
#define N_ARGS_HELPER1(args...) N_ARGS_HELPER2(args)
#define N_ARGS_HELPER2(x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, n, x...) n

#else
#error variadic macros for your compiler here
#endif

#define PASTE_HELPER(x,y) x ## y
#define PASTE(x,y) PASTE_HELPER(x,y)

#define STRINGIFY_HELPER(s) #s
#define STRINGIFY(x) STRINGIFY_HELPER(x)

#include <signal.h>
#define BREAK raise(SIGABRT);

#endif
