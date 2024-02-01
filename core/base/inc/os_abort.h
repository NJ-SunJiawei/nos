/************************************************************************
 *File name: os_abort.h
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#if !defined(OS_BASE_INSIDE) && !defined(OS_BASE_COMPILATION)
#error "This header file cannot be directly referenced."
#endif


#ifndef OS_ABORT_H
#define OS_ABORT_H

#ifdef __cplusplus
extern "C" {
#endif

OS_GNUC_NORETURN void os_abort(void);

#ifdef __cplusplus
}
#endif

#endif 
