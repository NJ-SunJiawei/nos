/************************************************************************
 *File name: os_random.h
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#if !defined(OS_BASE_INSIDE) && !defined(OS_BASE_COMPILATION)
#error "This header file cannot be directly referenced."
#endif

#ifndef OS_RAND_H
#define OS_RAND_H

#ifdef __cplusplus
extern "C" {
#endif

void os_random(void *buf, size_t buflen);
uint32_t os_random32(void);

#ifdef __cplusplus
}
#endif

#endif
