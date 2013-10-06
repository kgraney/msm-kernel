#include <linux/syscalls.h>
#include <asm/netlock.h>

SYSCALL_DEFINE1(netlock_acquire, netlock_t, type)
{
        return 0;
}

SYSCALL_DEFINE0(netlock_release)
{
        return 0;
}

int netlock_acquire(netlock_t type) { return sys_netlock_acquire(type); }

int netlock_release(void) { return sys_netlock_release(); }


