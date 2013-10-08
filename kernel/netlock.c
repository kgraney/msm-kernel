#include <linux/syscalls.h>
#include <linux/errno.h>
#include <linux/netlock.h>
#include <linux/semaphore.h>

static int __reg_count;
static int __excl_count;
static DEFINE_MUTEX(__mutex_1);
static DEFINE_MUTEX(__mutex_2);
static DEFINE_MUTEX(__mutex_3);

static DEFINE_SEMAPHORE(__reg);
static DEFINE_SEMAPHORE(__excl);

void netlock_init(void) {
        printk(KERN_INFO "netlock: initializing");
        printk(KERN_INFO "netlock: completed initialization");
}

/* Reference:
 *   Concurrent Control with "Readers" and "Writers"
 *   Courtnois, et. al. 1971
 */

SYSCALL_DEFINE1(netlock_acquire, netlock_t, type)
{
        printk(KERN_DEBUG "netlock: reg#:%d excl#:%d", __reg_count, __excl_count);
        switch (type) {
        case LOCK_R:
                /* try to obtain the regular (read) lock */
                printk(KERN_DEBUG "netlock: attempt at REGULAR lock (pid=%d)", current->pid);
                mutex_lock(&__mutex_3);
                if (down_interruptible(&__reg)) return -EINVAL;
                mutex_lock(&__mutex_1);
                __reg_count++;
                if (__reg_count == 1)
                        if (down_interruptible(&__excl)) return -EINVAL;
                mutex_unlock(&__mutex_1);
                up(&__reg);
                mutex_unlock(&__mutex_3);
                break;
        case LOCK_E:
                /* try to obtain the exclusive (write) lock */
                printk(KERN_DEBUG "netlock: attempt at EXCLUSIVE lock (pid=%d)", current->pid);
                mutex_lock(&__mutex_2);
                __excl_count++;
                if (__excl_count == 1)
                        if (down_interruptible(&__reg)) return -EINVAL;
                mutex_unlock(&__mutex_2);
                if (down_interruptible(&__excl)) return -EINVAL;
                break;
        default:
                return -EINVAL;
        }
        return 0;
}

SYSCALL_DEFINE0(netlock_release)
{
        netlock_t type = LOCK_R; /* (how do we know???) */

        /* Release lock type the current process holds; this works because it is
           assumed the locks are not recursive. */
        switch (type) {
        case LOCK_R:
                mutex_lock(&__mutex_1);
                __reg_count--;
                if (__reg_count == 0) up(&__excl);
                mutex_unlock(&__mutex_1);
                break;
        case LOCK_E:
                up(&__excl);
                mutex_lock(&__mutex_2);
                __excl_count--;
                if (__excl_count == 0) up(&__reg);
                mutex_unlock(&__mutex_2);
                break;
        default:
                printk(KERN_ERR "netlock: attempt to release invalid type");
                return -EINVAL;
        }
        return 0;
}

int netlock_acquire(netlock_t type) { return sys_netlock_acquire(type); }

int netlock_release(void) { return sys_netlock_release(); }

