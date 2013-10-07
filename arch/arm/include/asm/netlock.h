#ifndef _ASM_NETLOCK_H
#define _ASM_NETLOCK_H

enum __netlock_t {
        NET_LOCK_N, /* Placeholder for no lock */
        LOCK_R, /* Indicates regular lock */
        LOCK_E, /* Indicates exclusive lock */
};
typedef enum __netlock_t netlock_t;

/* Syscall 378. Acquire netlock. type indicates
   whether a regular or exclusive lock is needed. Returns 0 on success
   and -1 on failure.
 */
int netlock_acquire(netlock_t type);

/* Syscall 379. Release netlock. Return 0 on success and -1 on failure.
 */
int netlock_release(void);

struct netlock {
        atomic_t count;

        /* List of processes (struct task_struct*) waiting on the lock. */
        wait_queue_head_t wait_list;
};

#endif

