#include <linux/syscalls.h>

SYSCALL_DEFINE2(sys_ext4_cowcopy, const char __user*, src, const char __user*, dest)
{
	printk(KERN_DEBUG "COWCOPY: sys_ext4_cowcopy");
}

