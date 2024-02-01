/************************************************************************
 *File name: os_abort.c
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#include "system_config.h"

#if HAVE_EXECINFO_H
#include <execinfo.h>
#endif

#include "os_init.h"

OS_GNUC_NORETURN void os_abort(void)
{
#if HAVE_BACKTRACE
    int i;
    int nptrs;
    void *buffer[100];
    char **strings;

    nptrs = backtrace(buffer, OS_ARRAY_SIZE(buffer));
    OS_FATAL("backtrace() returned %d addresses", nptrs);

    strings = backtrace_symbols(buffer, nptrs);
    if (strings) {
        for (i = 1; i < nptrs; i++)
            OS_LOG_PRINT(OS_TLOG_FATAL, "%s\n", strings[i]);

        free(strings);
    }

    abort();
#elif defined(_WIN32)
    DebugBreak();
    abort();
    ExitProcess(127);
#else
    abort();
#endif
}
