/************************************************************************
 *File name: os_thread.h
 *Description:
 *
 *Current Version:
 *Author: Created by sjw --- 2024.01
************************************************************************/
#if !defined(OS_BASE_INSIDE) && !defined(OS_BASE_COMPILATION)
#error "This header file cannot be directly referenced."
#endif

#ifndef OS_THREAD_H
#define OS_THREAD_H

OS_BEGIN_EXTERN_C
/*
 * The following code is stolen from mongodb-c-driver
 * https://github.com/mongodb/mongo-c-driver/blob/master/src/libmongoc/src/mongoc/mongoc-thread-private.h
 */
#if !defined(_WIN32)
#define os_thread_mutex_t pthread_mutex_t
#define os_thread_mutex_init(_n) (void)pthread_mutex_init((_n), NULL)
#define os_thread_mutex_lock (void)pthread_mutex_lock
#define os_thread_mutex_trylock (void)pthread_mutex_trylock
#define os_thread_mutex_unlock (void)pthread_mutex_unlock
#define os_thread_mutex_destroy (void)pthread_mutex_destroy
#define os_thread_cond_t pthread_cond_t
#define os_thread_cond_init(_n) (void)pthread_cond_init((_n), NULL)
#define os_thread_cond_wait pthread_cond_wait
__attribute__((unused)) static os_inline int os_thread_cond_timedwait(
        pthread_cond_t *cond, pthread_mutex_t *mutex, os_time_t timeout)
{
    int r;
    struct timespec to;
    struct timeval tv;
    os_time_t usec;

    os_gettimeofday(&tv);

    usec = os_time_from_sec(tv.tv_sec) + tv.tv_usec + timeout;

    to.tv_sec = os_time_sec(usec);
    to.tv_nsec = os_time_usec(usec) * 1000;

    r = pthread_cond_timedwait(cond, mutex, &to);
    if (r == 0)
        return OS_OK; 
    else if (r == OS_ETIMEDOUT)
        return OS_TIMEUP;
    else 
        return OS_ERROR;
}
#define os_thread_cond_signal (void)pthread_cond_signal
#define os_thread_cond_broadcast pthread_cond_broadcast
#define os_thread_cond_destroy (void)pthread_cond_destroy
#define os_thread_id_t pthread_t
#define os_thread_join(_n) pthread_join((_n), NULL)
#else
#define os_thread_mutex_t CRITICAL_SECTION
#define os_thread_mutex_init InitializeCriticalSection
#define os_thread_mutex_trylock TryEnterCriticalSection
#define os_thread_mutex_lock EnterCriticalSection
#define os_thread_mutex_unlock LeaveCriticalSection
#define os_thread_mutex_destroy DeleteCriticalSection
#define os_thread_cond_t CONDITION_VARIABLE
#define os_thread_cond_init InitializeConditionVariable
#define os_thread_cond_wait(_c, _m) \
    os_thread_cond_timedwait ((_c), (_m), INFINITE)
static os_inline int os_thread_cond_timedwait(
    os_thread_cond_t *cond, os_thread_mutex_t *mutex, os_time_t timeout)
{
    int r;

    if (SleepConditionVariableCS(cond, mutex,
                (DWORD)os_time_to_msec(timeout))) {
        return OS_OK;
    } else {
        r = GetLastError();

        if (r == WAIT_TIMEOUT || r == ERROR_TIMEOUT) {
            return OS_TIMEUP;
        } else {
            return OS_ERROR;
        }
    }
}
#define os_thread_cond_signal WakeConditionVariable
#define os_thread_cond_broadcast WakeAllConditionVariable
static os_inline int os_thread_cond_destroy(os_thread_cond_t *_ignored)
{
   return 0;
}
#endif

typedef struct os_thread_s os_thread_t;

//os_thread_t *os_thread_create(void (*func)(void *), void *data);
//void os_thread_destroy(os_thread_t *thread, int delay);

OS_END_EXTERN_C

#endif
