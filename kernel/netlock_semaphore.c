#include <linux/netlock_semaphore.h>
#include <linux/sched.h>

void nl_sem_init(struct nl_semaphore* sem, int count) {
        sem->count = count;
        mutex_init(&sem->m);
}

void nl_up(struct nl_semaphore* sem) {
        mutex_lock(&sem->m);
        sem->count++;
        if (sem->count <= 0) {
                /* wakeup a process from the queue */
                wake_up_all(&sem->q);
        }
        mutex_unlock(&sem->m);
}

int nl_down(struct nl_semaphore* sem) {
        DEFINE_WAIT(__wait);

        mutex_lock(&sem->m);
        sem->count--;
        if (sem->count <= -1) {
                mutex_unlock(&sem->m);
                /* wait_event(sem->q, sem->count <= -1); */
                /* put the process on the wait queue */
                add_wait_queue(&sem->q, &__wait);
                while (sem->count <= -1) {
                        prepare_to_wait(&sem->q, &__wait, TASK_INTERRUPTIBLE);
                        if (signal_pending(current)) {
                                printk(KERN_DEBUG "netlock: exiting on signal");
                                return -EINTR;
                        }
                        printk(KERN_DEBUG "netlock: schedule %d", sem->count);
                        schedule();
                }
                finish_wait(&sem->q, &__wait);
        } else {
                mutex_unlock(&sem->m);
        }
        return 0;
}
