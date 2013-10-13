#include <linux/netlock_semaphore.h>

void nl_sem_init(struct nl_semaphore* sem, int count) {
        sem->count = count;
        mutex_init(&sem->sem_mutex);
}


