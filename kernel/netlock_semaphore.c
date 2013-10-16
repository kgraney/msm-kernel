#include <linux/netlock_semaphore.h>
#include <linux/sched.h>

void nl_sem_init(struct nl_semaphore* sem, int count) {
        sem->count = count;
}

void nl_up(struct nl_semaphore* sem) {
        nl_lock(&sem->m);
        sem->count++;
        if (sem->count <= 0) {
                /* wakeup a process from the queue */
                wake_up_all(&sem->q);
        }
        nl_unlock(&sem->m);
}

int nl_down(struct nl_semaphore* sem) {
        DEFINE_WAIT(__wait);

        nl_lock(&sem->m);
        sem->count--;
        if (sem->count <= -1) {
                nl_unlock(&sem->m);
                /* put the process on the wait queue */
                add_wait_queue(&sem->q, &__wait);
                while (sem->count <= -1) {
                        prepare_to_wait(&sem->q, &__wait, TASK_INTERRUPTIBLE);
                        if (signal_pending(current)) {
                                printk(KERN_DEBUG "netlock: exiting down on signal with error");
                                return -EINTR;
                        }
                        printk(KERN_DEBUG "netlock: schedule %d", sem->count);
                        schedule();
                }
                finish_wait(&sem->q, &__wait);
        } else {
                nl_unlock(&sem->m);
        }
        return 0;
}

int nl_lock(struct nl_lock* lock)
{
        DEFINE_WAIT(__wait);

        while (test_and_set_bit(NL_GUARD, (void*)&lock->flags));
        if (!test_bit(NL_FLAG, (void*)&lock->flags)) {
                /* the lock is immediately available */
                set_bit(NL_FLAG, (void*)&lock->flags);
                clear_bit(NL_GUARD, (void*)&lock->flags);
                return 0;
        } else {
                /* need to wait */
                add_wait_queue(&lock->q, &__wait);
                clear_bit(NL_GUARD, (void*)&lock->flags);
                while (!test_bit(NL_FLAG, (void*)&lock->flags)) {
                        prepare_to_wait(&lock->q, &__wait, TASK_INTERRUPTIBLE);
                        if (signal_pending(current)) {
                                printk(KERN_DEBUG "netlock: exiting lock on signal with error");
                                return -EINTR;
                        }
                        schedule();
                }
                finish_wait(&lock->q, &__wait);
                return 0;
        }
}

void nl_unlock(struct nl_lock* lock)
{
        while (test_and_set_bit(NL_GUARD, (void*)&lock->flags));
        if (!waitqueue_active(&lock->q))
                clear_bit(NL_FLAG, (void*)&lock->flags);
        else
                wake_up(&lock->q);

        clear_bit(NL_GUARD, (void*)&lock->flags);
}
