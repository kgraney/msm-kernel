#include <linux/syscalls.h>
#include <linux/errno.h>

SYSCALL_DEFINE1(fail, int, n)
{
	if (n > 0) {
		/* fail the nth system call */
		current->num_to_fault = n;
		current->flags |= PF_FAULT_CALL;
		return 0;

	} else if (n == 0) {
		/* stop fault injection session */
		current->flags &= ~PF_FAULT_CALL;
		return 0;
	}

	return -EINVAL;
}
