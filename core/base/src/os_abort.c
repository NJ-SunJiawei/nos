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

    strings = backtrace_symbols(buffer, nptrs);
    os_log(FATAL, "backtrace() returned %d addresses", nptrs);

    if (strings) {
        for (i = 0; i < nptrs; i++){
            os_print(FATAL, "BT[%d] : in Function %s",i, strings[i]);
        }

        free(strings);
    }
    abort_flush_data();
    abort();
#elif defined(_WIN32)
    DebugBreak();
    abort();
    ExitProcess(127);
#else
    abort();
#endif
}
