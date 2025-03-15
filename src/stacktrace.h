#ifndef __STACKTRACE_H_
#define __STACKTRACE_H_

#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>

inline static void print_stacktrace() {
    void* callstack[256];
    int i, frames = backtrace(callstack, sizeof(callstack));
    char** strs = backtrace_symbols(callstack, frames);
    for (i = 0; i < frames; ++i) {
        printf("%s\n", strs[i]);
    }
    free(strs);
}


#endif // __STACKTRACE_H_
