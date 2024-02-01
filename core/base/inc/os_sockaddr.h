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

/************************************************************************
 *File name: os_sockaddr.h
 *Description:
 *
 *Current Version:
 *Author: Copy by sjw --- 2024.01
************************************************************************/
#if !defined(OS_BASE_INSIDE) && !defined(OS_BASE_COMPILATION)
#error "This header file cannot be directly referenced."
#endif

#ifndef OS_SOCKADDR_H
#define OS_SOCKADDR_H

OS_BEGIN_EXTERN_C

typedef struct os_sockaddr_s os_sockaddr_t;
struct os_sockaddr_s {
    /* Reserved Area
     *   - Should not add any atrribute in this area.
     *
     *   e.g)
     *   struct sockaddr addr;
     *   ...
     *   sockaddr_len((os_sockaddr_t *)&addr);
     */
#define os_sa_family sa.sa_family
#define os_sin_port sin.sin_port
    union {
        struct sockaddr_storage ss;
        struct sockaddr_in sin;
        struct sockaddr_in6 sin6;
        struct sockaddr sa;
    };

    /*
     * First we created a 'hostname' variable.
     * If there is a name in the configuration file,
     * it is set in the 'hostname' of os_sockaddr_t.
     * Then, it immediately call getaddrinfo() to fill addr in os_sockaddr_t.
     *
     * When it was always possible to convert DNS to addr, that was no problem.
     * However, in some environments, such as Roaming, there are situations
     * where it is difficult to always change the DNS to addr.
     *
     * So, 'fqdn' was created for the purpose of first use in os_sbi_client_t.
     * 'fqdn' always do not change with addr.
     * This value is used as it is in the actual client connection.
     *
     * Note that 'hostname' is still in use for server or other client
     * except for os_sbi_client_t.
     */
    char *hostname;
    char *fqdn;

    os_sockaddr_t *next;
};

typedef struct os_ipsubnet_s {
    int family;

    uint32_t sub[4]; /* big enough for IPv4 and IPv6 addresses */
    uint32_t mask[4];
} os_ipsubnet_t;

int os_getaddrinfo(os_sockaddr_t **sa_list,
        int family, const char *hostname, uint16_t port, int flags);
int os_freeaddrinfo(os_sockaddr_t *sa_list);

int os_addaddrinfo(os_sockaddr_t **sa_list,
        int family, const char *hostname, uint16_t port, int flags);
int os_copyaddrinfo(
        os_sockaddr_t **dst, const os_sockaddr_t *src);
int os_filteraddrinfo(os_sockaddr_t **sa_list, int family);
int os_sortaddrinfo(os_sockaddr_t **sa_list, int family);

os_sockaddr_t *os_link_local_addr(const char *dev, const os_sockaddr_t *sa);
os_sockaddr_t *os_link_local_addr_by_dev(const char *dev);
os_sockaddr_t *os_link_local_addr_by_sa(const os_sockaddr_t *sa);
int os_filter_ip_version(os_sockaddr_t **addr,
        int no_ipv4, int no_ipv6, int prefer_ipv4);

#define OS_ADDRSTRLEN INET6_ADDRSTRLEN
#define OS_ADDR(__aDDR, __bUF) \
    os_inet_ntop(__aDDR, __bUF, OS_ADDRSTRLEN)
#define OS_PORT(__aDDR) \
    be16toh((__aDDR)->os_sin_port)
const char *os_inet_ntop(void *sa, char *buf, int buflen);
int os_inet_pton(int family, const char *src, void *sa);

socklen_t os_sockaddr_len(const void *sa);
bool os_sockaddr_is_equal(const void *p, const void *q);

int os_ipsubnet(os_ipsubnet_t *ipsub,
        const char *ipstr, const char *mask_or_numbits);

char *os_gethostname(os_sockaddr_t *addr);
char *os_ipstrdup(os_sockaddr_t *addr);

OS_END_EXTERN_C

#endif
