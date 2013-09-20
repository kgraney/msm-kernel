#include <linux/syscalls.h>
#include <linux/errno.h>
//#include <linux/sched.h>

SYSCALL_DEFINE1(fail, int, n)
{
	if (n < 0)
		return -EINVAL;

	current->num_to_fault = n;
	current->flags |= PF_FAULT_CALL;
	return 0;
}
