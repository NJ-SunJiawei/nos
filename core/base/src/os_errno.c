/************************************************************************
 *File name: os_errno.c
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/

#include "system_config.h"

#include "os_init.h"

char *os_strerror(os_err_t err, char *buf, size_t size)
{
#if defined(_WIN32)
    /*
     * The following code is stolen from APR Library
     * http://svn.apache.org/repos/asf/apr/apr/trunk/misc/unix/errorcodes.c
     */
    size_t len = 0, i;
    LPTSTR msg = (LPTSTR)buf;
    len = FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            err,
            MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), /* Default Language */
            msg,
            (DWORD)(size/sizeof(TCHAR)),
            NULL);

    if (len) {
        /* FormatMessage put the message in the buffer, but it may
         * have embedded a newline (\r\n), and possible more than one.
         * Remove the newlines replacing them with a space. This is not
         * as visually perfect as moving all the remaining message over,
         * but more efficient.
         */
        i = len;
        while (i) {
            i--;
            if ((buf[i] == '\r') || (buf[i] == '\n'))
                buf[i] = ' ';
        }
    }
    else {
        /* Windows didn't provide us with a message.  Even stuff like */
        os_snprintf(buf, size, "Unrecognized Win32 error code %d", (int)err);
    }

    return buf;
#elif defined(HAVE_STRERROR_R)

#if defined(STRERROR_R_CHAR_P)
    return strerror_r(err, buf, size);//GNU char *strerror_r(int errnum, char *buf, size_t buflen);
#else
    int ret = strerror_r(err, buf, size);//POSIX int strerror_r(int errnum, char *buf, size_t buflen);
    if (ret == 0)
        return buf;
    else
        return NULL;
#endif

#else
#error TODO
#endif
}
