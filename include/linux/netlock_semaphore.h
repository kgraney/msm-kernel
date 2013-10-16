#ifndef _LINUX_NETLOCK_SEMAPHORE_H
#define _LINUX_NETLOCK_SEMAPHORE_H

#include <linux/wait.h>
#include <linux/mutex.h>

#define NL_GUARD 0
#define NL_FLAG  1

/* Define a primitive lock */
struct nl_lock
{
        char flags; /* bit 0 = guard, bit 2 = flag */
        wait_queue_head_t q; /* queue for waiting threads */
};

/* Define a semaphore in terms of a simple mutex */
struct nl_semaphore
{
        int count; /* the count of the semaphore */
        struct nl_lock m; /* mutex around count */
        wait_queue_head_t q; /* queue for waiting threads */
};

#define __NETLOCK_INITIALIZER(name) { \
        .flags = 0, \
        .q = __WAIT_QUEUE_HEAD_INITIALIZER(name.q) }

#define DEFINE_NL_SEMAPHORE(name) \
        struct nl_semaphore name = { \
                .count = 1, \
                .m = __NETLOCK_INITIALIZER(name.m), \
                .q = __WAIT_QUEUE_HEAD_INITIALIZER(name.q) }

void nl_up(struct nl_semaphore* sem);
int nl_down(struct nl_semaphore* sem);

int nl_lock(struct nl_lock* lock);
void nl_unlock(struct nl_lock* lock);

#endif
