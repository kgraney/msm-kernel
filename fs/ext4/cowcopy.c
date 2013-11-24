#include <linux/syscalls.h>

SYSCALL_DEFINE2(ext4_cowcopy, const char __user, *src, const char __user, *dest)
{
	printk(KERN_DEBUG "COWCOPY: copy on write system call");
	return -EINVAL;
}
