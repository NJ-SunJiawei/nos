/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Copyright 2013 MongoDB, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/************************************************************************
 *File name: os_time.c
 *Description:
 *
 *Current Version:
 *Author: Modified by sjw --- 2024.01
************************************************************************/
#include "system_config.h"

#ifdef __APPLE__
#include <mach/clock.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#endif
#include "os_init.h"

/*
 * The following code is stolen from mongodb-c-driver
 * https://github.com/mongodb/mongo-c-driver/blob/master/src/libbson/src/bson/bson-clock.c
 */
int os_gettimeofday(struct timeval *tv)
{
#if defined(_WIN32)
#if defined(_MSC_VER)
#define DELTA_EPOCH_IN_MICROSEC 11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSEC 11644473600000000ULL
#endif
    FILETIME ft;
    uint64_t tmp = 0;

    /*
     * The const value is shamelessy stolen from
     * http://www.boost.org/doc/libs/1_55_0/boost/chrono/detail/inlined/win/chrono.hpp
     *
     * File times are the number of 100 nanosecond intervals elapsed since
     * 12:00 am Jan 1, 1601 UTC.  I haven't check the math particularly hard
     *
     * ...  good luck
     */

    if (tv) {
        GetSystemTimeAsFileTime (&ft);

        /* pull out of the filetime into a 64 bit uint */
        tmp |= ft.dwHighDateTime;
        tmp <<= 32;
        tmp |= ft.dwLowDateTime;

        /* convert from 100's of nanosecs to microsecs */
        tmp /= 10;

        /* adjust to unix epoch */
        tmp -= DELTA_EPOCH_IN_MICROSEC;

        tv->tv_sec = (long) (tmp / 1000000UL);
        tv->tv_usec = (long) (tmp % 1000000UL);
    }

    return 0;
#else
    int rc = gettimeofday(tv, NULL);
    os_assert(rc == 0);
    return 0;
#endif
}

os_time_t os_time_now(void)
{
    int rc;
    struct timeval tv;
    //和clock_gettime函数类似，gettimeofday通过struct timeval结构返回日历时间，该结构把时间表示为秒和微妙
    rc = os_gettimeofday(&tv);
    os_assert(rc == 0);

    return     os_time_from_sec(tv.tv_sec) + tv.tv_usec;
}

/* The following code is stolen from APR library */
int os_time_from_lt(os_time_t *t, struct tm *tm, int tm_usec)
{
    os_time_t year = tm->tm_year;
    os_time_t days;
    static const int dayoffset[12] =
    {306, 337, 0, 31, 61, 92, 122, 153, 184, 214, 245, 275};

    if (tm->tm_mon < 0 || tm->tm_mon >= 12)
        return OS_ERROR;

    /* shift new year to 1st March in order to make leap year calc easy */

    if (tm->tm_mon < 2)
        year--;

    /* Find number of days since 1st March 1900 (in the Gregorian calendar). */

    days = year * 365 + year / 4 - year / 100 + (year / 100 + 3) / 4;
    days += dayoffset[tm->tm_mon] + tm->tm_mday - 1;
    days -= 25508;              /* 1 jan 1970 is 25508 days since 1 mar 1900 */
    days = ((days * 24 + tm->tm_hour) * 60 + tm->tm_min) * 60 + tm->tm_sec;

    if (days < 0) {
        return OS_ERROR;
    }
    *t = days * OS_USEC_PER_SEC + tm_usec;

    return OS_OK;
}

int os_time_from_gmt(os_time_t *t, struct tm *tm, int tm_usec)
{
    int status = os_time_from_lt(t, tm, tm_usec);
    if (status == OS_OK)
        *t -= (os_time_t) tm->tm_gmtoff * OS_USEC_PER_SEC;
    return status;
}

/* RFC 5905 A.1.1, A.4
 * PFCP entity uses NTP timestamp(1900), but Open5GS uses UNIX(1970).
 *
 * One is the offset between the two epochs.
 * Unix uses an epoch located at 1/1/1970-00:00h (UTC) and
 * NTP uses 1/1/1900-00:00h. This leads to an offset equivalent
 * to 70 years in seconds (there are 17 leap years
 * between the two dates so the offset is
 *
 *  (70*365 + 17)*86400 = 2208988800
 *
 * to be substracted from NTP time to get Unix struct timeval.
 */
uint32_t os_time_ntp32_now(void)
{
    int rc;
    struct timeval tv;

    rc = os_gettimeofday(&tv);
    os_assert(rc == 0);

    return os_time_to_ntp32(os_time_from_sec(tv.tv_sec) + tv.tv_usec);
}

os_time_t os_time_from_ntp32(uint32_t ntp_timestamp)
{
    if (ntp_timestamp < OS_1970_1900_SEC_DIFF)
        return 0;
    return os_time_from_sec(ntp_timestamp - OS_1970_1900_SEC_DIFF);
}

uint32_t os_time_to_ntp32(os_time_t time)
{
    return (time / OS_USEC_PER_SEC) + OS_1970_1900_SEC_DIFF;
}

int os_timezone(void)
{
#if defined(_WIN32)
    u_long                 n;
    TIME_ZONE_INFORMATION  tz;

    n = GetTimeZoneInformation(&tz);

    switch (n) {
    case TIME_ZONE_ID_UNKNOWN:
        /* Bias = UTC - local time in minutes
         * tm_gmtoff is seconds east of UTC
         */
        return tz.Bias * -60;
    case TIME_ZONE_ID_STANDARD:
        return (tz.Bias + tz.StandardBias) * -60;
    case TIME_ZONE_ID_DAYLIGHT:
        return (tz.Bias + tz.DaylightBias) * -60;
    default:
        os_assert_if_reached();
        return 0;
    }
#else
    struct timeval tv;
    struct tm tm;
    int ret;

    ret = os_gettimeofday(&tv);
    os_assert(ret == 0);

    os_localtime(tv.tv_sec, &tm);

    return tm.tm_gmtoff;
#endif
}

os_time_t os_get_monotonic_time(void)
{
#if defined(HAVE_CLOCK_GETTIME) && defined(CLOCK_MONOTONIC)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return os_time_from_sec(ts.tv_sec) + ts.tv_nsec / 1000UL;
#elif defined(__APPLE__)
    static mach_timebase_info_data_t info = {0};
    static double ratio = 0.0;

    if (!info.denom) {
        /* the value from mach_absolute_time () * info.numer / info.denom
        * is in nano seconds. So we have to divid by 1000.0 to get micro
        * seconds*/
        mach_timebase_info(&info);
        ratio = (double) info.numer / (double) info.denom / 1000.0;
    }

    return mach_absolute_time() * ratio;
#elif defined(_WIN32)
    /* Despite it's name, this is in milliseconds! */
    os_time_t ticks = GetTickCount64();
    return (ticks * 1000L);
#elif defined(__hpux__)
    os_time_t nanosec = gethrtime();
    return (nanosec / 1000UL);
#else
#warning "Monotonic clock is not yet supported on your platform."
    struct timeval tv;

    os_gettimeofday(&tv);
    return os_time_from_sec(tv.tv_sec) + tv.tv_usec;
#endif
}

void os_localtime(time_t s, struct tm *tm)
{
    os_assert(tm);
    memset(tm, 0, sizeof(*tm));

#if (HAVE_LOCALTIME_R)
    (void)localtime_r(&s, tm);
#else
    struct tm *t;

    t = localtime(&s);
    *tm = *t;
#endif
}

void os_gmtime(time_t s, struct tm *tm)
{
    os_assert(tm);
    memset(tm, 0, sizeof(*tm));

#if (HAVE_LOCALTIME_R)
    (void)gmtime_r(&s, tm);
#else
    struct tm *t;

    t = gmtime(&s);
    *tm = *t;
#endif
}

void os_msleep(time_t msec)
{
#if defined(_WIN32)
    Sleep(msec);
#else
    os_usleep(msec * 1000);
#endif
}

void os_usleep(time_t usec)
{
#if defined(_WIN32)
    Sleep(usec ? (1 + (usec - 1) / 1000) : 0);
#else
    struct timespec req, rem;
    req.tv_sec = usec / OS_USEC_PER_SEC;
    req.tv_nsec = (usec % OS_USEC_PER_SEC) * 1000;
    while (nanosleep(&req, &rem) == -1 && errno == EINTR)
        req = rem;
#endif
}
