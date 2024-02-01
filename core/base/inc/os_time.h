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
 *File name: os_time.h
 *Description:
 *
 *Current Version:
 *Author: Modified by sjw --- 2024.01
************************************************************************/
#if !defined(OS_BASE_INSIDE) && !defined(OS_BASE_COMPILATION)
#error "This header file cannot be directly referenced."
#endif

#ifndef OS_TIME_H
#define OS_TIME_H

#ifdef __cplusplus
extern "C" {
#endif

/** number of microseconds in the interval */
typedef int64_t os_interval_time_t;

#define OS_INFINITE_TIME (-1)
#define OS_NO_WAIT_TIME (0)

/** number of microseconds per second */
#define OS_USEC_PER_SEC (1000000LL)

/** @return os_time_t as a second */
#define os_time_sec(time) ((time) / OS_USEC_PER_SEC)

/** @return os_time_t as a usec */
#define os_time_usec(time) ((time) % OS_USEC_PER_SEC)

/** @return os_time_t as a msec */
#define os_time_msec(time) (((time) / 1000) % 1000)

/** @return os_time_t as a msec */
#define os_time_to_msec(time) ((time) ? (1 + ((time) - 1) / 1000) : 0)

/** @return milliseconds as an os_time_t */
#define os_time_from_msec(msec) ((os_time_t)(msec) * 1000)

/** @return seconds as an os_time_t */
#define os_time_from_sec(sec) ((os_time_t)(sec) * OS_USEC_PER_SEC)

int os_gettimeofday(struct timeval *tv);

os_time_t os_time_now(void); /* This returns GMT */
int os_time_from_lt(os_time_t *t, struct tm *tm, int tm_usec);
int os_time_from_gmt(os_time_t *t, struct tm *tm, int tm_usec);

#define OS_1970_1900_SEC_DIFF 2208988800UL /* 1970 - 1900 in seconds */
uint32_t os_time_ntp32_now(void); /* This returns NTP timestamp (1900) */
os_time_t os_time_from_ntp32(uint32_t ntp_timestamp);
uint32_t os_time_to_ntp32(os_time_t time);

/** @return number of microseconds since an arbitrary point */
os_time_t os_get_monotonic_time(void);
/** @return the GMT offset in seconds */
int os_timezone(void);

void os_localtime(time_t s, struct tm *tm);
void os_gmtime(time_t s, struct tm *tm);

void os_msleep(time_t msec);
void os_usleep(time_t usec);

#define os_mktime mktime
#define os_strptime strptime
#define os_strftime strftime

#ifdef __cplusplus
}
#endif

#endif /* OS_TIME_H */
