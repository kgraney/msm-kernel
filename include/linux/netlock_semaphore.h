#ifndef _LINUX_NETLOCK_SEMAPHORE_H
#define _LINUX_NETLOCK_SEMAPHORE_H

#include <linux/wait.h>
#include <linux/mutex.h>

struct nl_mutex
{
        int flag; /* mutual exclusion status:   0 -> available; 1 -> unavailable */
        int guard; /* guard to avoid losing wakups:0 -> available; 1 -> unavailable */
        wait_queue_t q; /* queue for waiting threads */
};

/* Define a semaphore in terms of a simple mutex */
struct nl_semaphore
{
        int count; /* the count of the semaphore */
        struct mutex sem_mutex; /* mutex around count */
};

#endif
