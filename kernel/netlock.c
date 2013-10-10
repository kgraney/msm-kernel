#include <linux/syscalls.h>
#include <linux/errno.h>
#include <linux/netlock.h>
#include <linux/semaphore.h>
#include <linux/radix-tree.h>
#include <linux/slab.h>
#include <linux/list.h>

static int __reg_count;
static int __excl_count;
static DEFINE_MUTEX(__mutex_1);
static DEFINE_MUTEX(__mutex_2);
static DEFINE_MUTEX(__mutex_3);
static DEFINE_MUTEX(__mutex_radix);

static DEFINE_SEMAPHORE(__reg);
static DEFINE_SEMAPHORE(__excl);

static struct radix_tree_root __proc_locks; /* what locks each process holds */

void netlock_init(void) {
        printk(KERN_INFO "netlock: initializing");
        INIT_RADIX_TREE(&__proc_locks, GFP_ATOMIC);
        printk(KERN_INFO "netlock: completed initialization");
}

int add_entry(netlock_t type) {
        struct __netlock_record* new_record, * head_record;
        new_record = kmalloc(sizeof(*new_record), GFP_KERNEL);
        if (!new_record) return -ENOSPC;
        new_record->type = type;

        mutex_lock(&__mutex_radix);
        head_record = radix_tree_lookup(&__proc_locks, current->pid);
        if (!head_record) {
                INIT_LIST_HEAD(&new_record->list);
                radix_tree_insert(&__proc_locks, current->pid, (void*)new_record);
        } else {
               list_add_tail(&new_record->list, &head_record->list);
        }
        mutex_unlock(&__mutex_radix);

        return 0;
}

/* Reference for semaphore pattern:
 *   Concurrent Control with "Readers" and "Writers"
 *   Courtnois, et. al. 1971
 */

SYSCALL_DEFINE1(netlock_acquire, netlock_t, type)
{
        int entry_return;

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

                /* store bookkeeping information */
                entry_return = add_entry(type);
                if (entry_return < 0) { /* TODO: cleanup and fail */
                }
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

                /* store bookkeeping information */
                entry_return = add_entry(type);
                if (entry_return < 0) { /* TODO: cleanup and fail */
                }
                break;

        default:
                printk(KERN_ERR "netlock: attempt to acquire invalid type");
                return -EINVAL;
        }
        printk(KERN_DEBUG "netlock: reg#:%d excl#:%d", __reg_count, __excl_count);
        return 0;
}

SYSCALL_DEFINE0(netlock_release)
{
        struct __netlock_record* head_record, *record;
        netlock_t type;

        mutex_lock(&__mutex_radix);
        head_record = radix_tree_lookup(&__proc_locks, current->pid);
        record = list_entry(head_record->list.prev, struct __netlock_record, list);
        type = record->type;

        list_del(&record->list);
        kfree(record);
        if (head_record == record) /* list empty, clear radix tree entry */
                radix_tree_delete(&__proc_locks, current->pid);

        /* Release lock type the current process last acquired, which is
           determined above. */
        switch (type) {
        case LOCK_R:
                printk(KERN_DEBUG "netlock: release REGULAR lock (pid=%d)", current->pid);
                mutex_lock(&__mutex_1);
                __reg_count--;
                if (__reg_count == 0) up(&__excl);
                mutex_unlock(&__mutex_1);
                break;
        case LOCK_E:
                printk(KERN_DEBUG "netlock: release EXCLUSIVE lock (pid=%d)", current->pid);
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
        printk(KERN_DEBUG "netlock: reg#:%d excl#:%d", __reg_count, __excl_count);
        return 0;
}

int netlock_acquire(netlock_t type) { return sys_netlock_acquire(type); }

int netlock_release(void) { return sys_netlock_release(); }

