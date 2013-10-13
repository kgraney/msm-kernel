#ifndef _LINUX_NETLOCK_SEMAPHORE_H
#define _LINUX_NETLOCK_SEMAPHORE_H

#include <linux/wait.h>
#include <linux/mutex.h>

/* Define a semaphore in terms of a simple mutex */
struct nl_semaphore
{
        int count; /* the count of the semaphore */
        struct mutex m; /* mutex around count */
        wait_queue_head_t q; /* queue for waiting threads */
};

#define DEFINE_NL_SEMAPHORE(name) \
        struct nl_semaphore name = { \
                .count = 1, \
                .m = __MUTEX_INITIALIZER(name.m), \
                .q = __WAIT_QUEUE_HEAD_INITIALIZER(name.q) \
                }

void nl_up(struct nl_semaphore* sem);
int nl_down(struct nl_semaphore* sem);

#endif
