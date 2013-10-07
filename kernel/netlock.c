#include <linux/syscalls.h>
#include <linux/errno.h>
#include <linux/netlock.h>

struct netlock __netlock_reg;
struct netlock __netlock_excl;

static void __netlock_init(struct netlock* lock, netlock_t type);

void netlock_init(void) {
        __netlock_init(&__netlock_reg, LOCK_R);
        __netlock_init(&__netlock_excl, LOCK_E);
}

void __netlock_init(struct netlock* lock, netlock_t type)
{
        init_waitqueue_head(&lock->wait_list);

        switch (type) {
        case LOCK_R:
                atomic_set(&lock->count, 0);
                break;
        case LOCK_E:
                atomic_set(&lock->count, 1);
                break;
        default:
                printk(KERN_DEBUG "invalid type to __netlock_init");
                dump_stack();
        }
}

SYSCALL_DEFINE1(netlock_acquire, netlock_t, type)
{
        switch (type) {
        case LOCK_R:
                /* try to obtain the regular (read) lock */
        case LOCK_E:
                /* try to obtain the exclusive (write) lock */
        default:
                return -EINVAL;
        }
}

SYSCALL_DEFINE0(netlock_release)
{
        /* release whatever lock the current process holds */
        /* (how do we know???) */
        return 0;
}

int netlock_acquire(netlock_t type) { return sys_netlock_acquire(type); }

int netlock_release(void) { return sys_netlock_release(); }

