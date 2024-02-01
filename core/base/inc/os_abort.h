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

OS_BEGIN_EXTERN_C

OS_GNUC_NORETURN void os_abort(void);

OS_END_EXTERN_C

#endif 
