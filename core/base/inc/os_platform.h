/************************************************************************
 *File name: os_platform.h
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#if !defined(OS_BASE_INSIDE) && !defined(OS_BASE_COMPILATION)
#error "This header file cannot be directly referenced."
#endif

#ifndef OS_COMPAT_H
#define OS_COMPAT_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#define __OS_FUNC__ __FUNCTION__
#else
#define __OS_FUNC__ (const char *)__func__
#endif

//////////////////////////////////////////////////////////////
#if defined(_WIN32) || defined(_MSC_VER)

#include <winsock2.h>
//#include <ws2tcpip.h> /* IPv6 */
#if _MSC_VER < 1500
/* work around bug in msvc 2005 code analysis http://connect.microsoft.com/VisualStudio/feedback/ViewFeedback.aspx?FeedbackID=99397 */
#pragma warning(push)
#pragma warning(disable:6011)
#include <Ws2tcpip.h>
#pragma warning(pop)
#else
/* work around for warnings in vs 2010 */
#pragma warning (disable:6386)
#include <Ws2tcpip.h>
#pragma warning (default:6386)
#endif


#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#else
#include <windows.h>
#endif

/* For structs needed by GetAdaptersAddresses */
#if defined(_WIN32_WINNT) && (_WIN32_WINNT < 0x0600)
#undef _WIN32_WINNT
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#if !defined(_STDINT) && !defined(uint32_t)
typedef unsigned __int8 uint8_t;         // u char
typedef unsigned __int16 uint16_t;    // u short int
typedef unsigned __int32 uint32_t;    // u int(32/64) /long(32) int
typedef unsigned __int64 uint64_t;    // u long(64) int /long long(64) int
typedef __int8 int8_t;
typedef __int16 int16_t;
typedef __int32 int32_t;
typedef __int64 int64_t;
typedef unsigned long in_addr_t;
#endif
typedef int pid_t;
typedef int uid_t;
typedef int gid_t;
#define PACKED
#include <io.h>
#define strcasecmp(s1, s2) stricmp(s1, s2)
#define strncasecmp(s1, s2, n) strnicmp(s1, s2, n)

#else /* !defined(_WIN32) */
/* packed attribute */
#if (defined __SUNPRO_CC) || defined(__SUNPRO_C)
#define PACKED
#endif
#ifndef PACKED
#define PACKED __attribute__ ((__packed__))
#endif

#include <strings.h>
#include <stdbool.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/socket.h>
/* setuid, setgid */
#include <unistd.h>

/* getgrnam, getpwnam */
#include <pwd.h>
#include <grp.h>
#include <inttypes.h>
#include <unistd.h>

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <assert.h>
#include <setjmp.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

#if 0
/* for apr_pool_create and apr_pool_destroy */
/* functions only used in this file so not exposed */
#include <apr_pools.h>

/* for apr_hash_make, apr_hash_pool_get, apr_hash_set */
/* functions only used in this file so not exposed */
#include <apr_hash.h>

/* for apr_pvsprintf */
/* function only used in this file so not exposed */
#include <apr_strings.h>

/* for apr_initialize and apr_terminate */
/* function only used in this file so not exposed */
#include <apr_general.h>

#include <apr_portable.h>

//#include <apr_thread_pool.h>
#endif

#ifdef HAVE_MLOCKALL
#include <sys/mman.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

////////////////////////////////////////////////
#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN 1234
#endif
#ifndef __BIG_ENDIAN
#define __BIG_ENDIAN 4321
#endif
#ifndef __BYTE_ORDER
#ifdef OS_BYTE_ORDER
#define __BYTE_ORDER OS_BYTE_ORDER
/* solaris */
#elif defined(__SVR4) && defined(__sun) && defined(_BIG_ENDIAN)
#define __BYTE_ORDER __BIG_ENDIAN
/* BSD */
#elif defined(BYTE_ORDER) && (BYTE_ORDER == BIG_ENDIAN)
#define __BYTE_ORDER __BIG_ENDIAN
#else
#define __BYTE_ORDER __LITTLE_ENDIAN
#endif
#endif

//////////////////////////////////////////////
#if __GNUC__ >= 3
#define PRINTF_FUNCTION(fmtstr,vars) __attribute__((format(printf,fmtstr,vars)))
#else
#define PRINTF_FUNCTION(fmtstr,vars)
#endif
#ifdef OS_INT32
typedef OS_INT32 os_int32_t;
#else
typedef int32_t os_int32_t;
#endif


#ifdef OS_SIZE_T
typedef OS_SIZE_T os_size_t;
#else
typedef uintptr_t os_size_t;
#endif

#ifdef OS_SSIZE_T
typedef OS_SSIZE_T os_ssize_t;
#else
typedef intptr_t os_ssize_t;
#endif

///////////////////////////////////////////////////////////////
#ifdef WIN32

#ifdef _WIN64
#define OS_SSIZE_T_FMT          "lld"
#define OS_SIZE_T_FMT           "lld"
#define FS_64BIT 1
#else
#define OS_SSIZE_T_FMT          "d"
#define OS_SIZE_T_FMT           "d"
#endif

#define OS_INT64_T_FMT          "lld"
#define OS_UINT64_T_FMT         "llu"

#ifndef TIME_T_FMT
#define TIME_T_FMT OS_INT64_T_FMT
#endif

#else
#ifndef OS_SSIZE_T_FMT
#define OS_SSIZE_T_FMT          (sizeof (os_ssize_t) == sizeof (long) ? "ld" : sizeof (os_ssize_t) == sizeof (int) ? "d" : "lld")
#endif

#ifndef OS_SIZE_T_FMT
#define OS_SIZE_T_FMT           (sizeof (os_size_t) == sizeof (long) ? "lu" : sizeof (os_size_t) == sizeof (int) ? "u" : "llu")
#endif

#ifndef OS_INT64_T_FMT
#define OS_INT64_T_FMT          (sizeof (long) == 8 ? "ld" : "lld")
#endif

#ifndef OS_UINT64_T_FMT
#define OS_UINT64_T_FMT         (sizeof (long) == 8 ? "lu" : "llu")
#endif

#ifndef TIME_T_FMT
#if defined(__FreeBSD__) && SIZEOF_VOIDP == 4
#define TIME_T_FMT "d"
#else
#if __USE_TIME_BITS64
#define TIME_T_FMT OS_INT64_T_FMT
#else
#define TIME_T_FMT "ld"
#endif
#endif
#endif


#if UINTPTR_MAX == 0xffffffffffffffff
#define FS_64BIT 1
#endif

#endif

#if defined(__sun__) && (defined(__x86_64) || defined(__arch64__))
#define OS_TIME_T_FMT OS_SIZE_T_FMT
#else
#define OS_TIME_T_FMT OS_INT64_T_FMT
#endif

/////////////////////////////////////////////////////////////
#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE (!FALSE)
#endif

#ifndef os_sys_assert
#ifdef PVS_STUDIO // Mute PVS FALSE ALARM
#define os_sys_assert(expr) do {if (!(expr)) {fprintf(stderr, "ERR\n"); abort();}} while (0)
#else
#define os_sys_assert(expr) assert(expr)
#endif
#endif

#ifndef __ATTR_SAL
/* used for msvc code analysis */
/* http://msdn2.microsoft.com/en-us/library/ms235402.aspx */
#define _In_
#define _In_z_
#define _In_opt_z_
#define _In_opt_
#define _Printf_format_string_
#define _Ret_opt_z_
#define _Ret_z_
#define _Out_opt_
#define _Out_
#define _Check_return_
#define _Inout_
#define _Inout_opt_
#define _In_bytecount_(x)
#define _Out_opt_bytecapcount_(x)
#define _Out_bytecapcount_(x)
#define _Ret_
#define _Post_z_
#define _Out_cap_(x)
#define _Out_z_cap_(x)
#define _Out_ptrdiff_cap_(x)
#define _Out_opt_ptrdiff_cap_(x)
#define _Post_count_(x)
#endif

#define _ENTER_API_
#define _EXIT_API_
#define _CONF_API_
#define _API_

#ifdef PRIVATE
#undef PRIVATE
#define PRIVATE   static
#else
#define PRIVATE   static
#endif

//////////////////////////////////////////////////////
#ifdef __GNUC__
#define OS_GNUC_CHECK_VERSION(major, minor) \
    ((__GNUC__ > (major)) || \
     ((__GNUC__ == (major)) && (__GNUC_MINOR__ >= (minor))))
#else
#define OS_GNUC_CHECK_VERSION(major, minor) 0
#endif

#if defined(_MSC_VER)
#define os_inline __inline
#else
#define os_inline __inline__
#endif

#if defined(_WIN32)
#define OS_FUNC __FUNCTION__
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ < 199901L
#define OS_FUNC __FUNCTION__
#else
#define OS_FUNC __func__
#endif

#if defined(__GNUC__)
#define os_likely(x) __builtin_expect (!!(x), 1)
#define os_unlikely(x) __builtin_expect (!!(x), 0)
#else
#define os_likely(v) v
#define os_unlikely(v) v
#endif

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#if !defined (__clang__) && OS_GNUC_CHECK_VERSION (4, 4)
#define OS_GNUC_PRINTF(f, v) __attribute__ ((format (gnu_printf, f, v)))
#else
#define OS_GNUC_PRINTF(f, v) __attribute__ ((format (__printf__, f, v)))
#endif
#define OS_GNUC_NORETURN __attribute__((__noreturn__))
#else
#define OS_GNUC_PRINTF(f, v) 
#define OS_GNUC_NORETURN
#endif

#if __GNUC__ > 6
#define OS_GNUC_FALLTHROUGH __attribute__ ((fallthrough))
#else
#define OS_GNUC_FALLTHROUGH
#endif

#define OS_GNUC_PACKED __attribute__ ((__packed__))

#if defined(_WIN32)
#define htole16(x) (x)
#define htole32(x) (x)
#define htole64(x) (x)
#define le16toh(x) (x)
#define le32toh(x) (x)
#define le64toh(x) (x)

#define htobe16(x) htons((x))
#define htobe32(x) htonl((x))
#define htobe64(x) htonll((x))
#define be16toh(x) ntohs((x))
#define be32toh(x) ntohl((x))
#define be64toh(x) ntohll((x))

#elif defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define htole16(x) OSSwapHostToLittleInt16((x))
#define htole32(x) OSSwapHostToLittleInt32((x))
#define htole64(x) OSSwapHostToLittleInt64((x))
#define le16toh(x) OSSwapLittleToHostInt16((x))
#define le32toh(x) OSSwapLittleToHostInt32((x))
#define le64toh(x) OSSwapLittleToHostInt64((x))

#define htobe16(x) OSSwapHostToBigInt16((x))
#define htobe32(x) OSSwapHostToBigInt32((x))
#define htobe64(x) OSSwapHostToBigInt64((x))
#define be16toh(x) OSSwapBigToHostInt16((x))
#define be32toh(x) OSSwapBigToHostInt32((x))
#define be64toh(x) OSSwapBigToHostInt64((x))

#elif defined(__FreeBSD__)
#include <sys/endian.h>

#elif defined(__linux__)
#include <endian.h>

#endif

#ifndef WORDS_BIGENDIAN
#if OS_BYTE_ORDER == OS_BIG_ENDIAN
#define WORDS_BIGENDIAN 1
#endif
#endif

typedef struct os_uint24_s {
    uint32_t v:24;
}  __attribute__ ((packed)) os_uint24_t;

PRIVATE os_inline os_uint24_t os_be24toh(os_uint24_t x)
{
    uint32_t tmp = x.v;
    tmp = be32toh(tmp);
    x.v = tmp >> 8;
    return x;
}

PRIVATE os_inline os_uint24_t os_htobe24(os_uint24_t x)
{
    uint32_t tmp = x.v;
    tmp = htobe32(tmp);
    x.v = tmp >> 8;
    return x;
}

#if OS_BYTE_ORDER == OS_BIG_ENDIAN
#define ED2(x1, x2) x1 x2
#define ED3(x1, x2, x3) x1 x2 x3
#define ED4(x1, x2, x3, x4) x1 x2 x3 x4
#define ED5(x1, x2, x3, x4, x5) x1 x2 x3 x4 x5
#define ED6(x1, x2, x3, x4, x5, x6) x1 x2 x3 x4 x5 x6
#define ED7(x1, x2, x3, x4, x5, x6, x7) x1 x2 x3 x4 x5 x6 x7
#define ED8(x1, x2, x3, x4, x5, x6, x7, x8) x1 x2 x3 x4 x5 x6 x7 x8
#else
#define ED2(x1, x2) x2 x1
#define ED3(x1, x2, x3) x3 x2 x1
#define ED4(x1, x2, x3, x4) x4 x3 x2 x1
#define ED5(x1, x2, x3, x4, x5) x5 x4 x3 x2 x1
#define ED6(x1, x2, x3, x4, x5, x6) x6 x5 x4 x3 x2 x1
#define ED7(x1, x2, x3, x4, x5, x6, x7) x7 x6 x5 x4 x3 x2 x1
#define ED8(x1, x2, x3, x4, x5, x6, x7, x8) x8 x7 x6 x5 x4 x3 x2 x1
#endif

// for gcc judge
#define OS_STATIC_ASSERT(expr) typedef char dummy_for_os_static_assert##__LINE__[(expr) ? 1 : -1]

#define OS_ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define OS_STRINGIFY(n)            OS_STRINGIFY_HELPER(n)
#define OS_STRINGIFY_HELPER(n)     #n

#define OS_PASTE(n1, n2)           OS_PASTE_HELPER(n1, n2)
#define OS_PASTE_HELPER(n1, n2)    n1##n2

#define OS_INET_NTOP(src, dst) \
    inet_ntop(AF_INET, (void *)(uintptr_t)(src), (dst), INET_ADDRSTRLEN)
#define OS_INET6_NTOP(src, dst) \
    inet_ntop(AF_INET6, (void *)(src), (dst), INET6_ADDRSTRLEN)

#define os_max(x , y)  (((x) > (y)) ? (x) : (y))
#define os_min(x , y)  (((x) < (y)) ? (x) : (y))

#if defined(_WIN32)
#define OS_IS_DIR_SEPARATOR(c) ((c) == OS_DIR_SEPARATOR || (c) == '/')
#else
#define OS_IS_DIR_SEPARATOR(c) ((c) == OS_DIR_SEPARATOR)
#endif

#define os_container_of(ptr, type, member) \
    (type *)((unsigned char *)ptr - offsetof(type, member))

#ifndef SWITCH_CASE_INIT
#define SWITCH_CASE_INIT
    #define SWITCH(X)    {char *__switch_p__,  __switch_next__; \
                          for (__switch_p__ = \
                                  X ? (char *)X : (char *)"OS_SWITCH_NULL", \
                                  __switch_next__ = 1; \
                              __switch_p__; \
                              __switch_p__ = 0, __switch_next__ = 1) { {
    #define CASE(X)            } if (!__switch_next__ || \
                                     (__switch_next__ = \
                                         strcmp(__switch_p__, X)) == 0) {
    #define DEFAULT            } {
    #define END          }}}
#endif

#define OS_FILE_LINE __FILE__ ":" OS_STRINGIFY(__LINE__)

#define os_uint64_to_uint32(x) ((x >= 0xffffffffUL) ? 0xffffffffU : x)

#define OS_OBJECT_REF(__oBJ) \
    ((__oBJ)->reference_count)++, \
    OS_DEBUG("[REF] %d", ((__oBJ)->reference_count))
#define OS_OBJECT_UNREF(__oBJ) \
    OS_DEBUG("[UNREF] %d", ((__oBJ)->reference_count)), \
    ((__oBJ)->reference_count)--
#define OS_OBJECT_IS_REF(__oBJ) ((__oBJ)->reference_count > 1)

#ifdef __cplusplus
}
#endif

#endif
