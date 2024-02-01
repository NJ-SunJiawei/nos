/************************************************************************
 *File name: os_notify.h
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#if !defined(OS_BASE_INSIDE) && !defined(OS_BASE_COMPILATION)
#error "This header file cannot be directly referenced."
#endif

#ifndef OS_NOTIFY_H
#define OS_NOTIFY_H

OS_BEGIN_EXTERN_C

void os_notify_init(os_pollset_t *pollset);
void os_notify_final(os_pollset_t *pollset);
int os_notify_pollset(os_pollset_t *pollset);

OS_END_EXTERN_C

#endif
