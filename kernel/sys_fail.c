#include <linux/syscalls.h>
#include <linux/errno.h>

SYSCALL_DEFINE1(fail, int, n)
{
	if (n > 0) {
		/* fail the nth system call */
		current->num_to_fault = n;
		current->flags |= PF_FAULT_CALL;
		return 0;

	} else if (n == 0 && (current->flags & PF_FAULT_CALL)) {
		/* stop fault injection session */
		current->flags &= ~PF_FAULT_CALL;
		return 0;
	}

	return -EINVAL;
}

long should_fail(void) {
	return (current->flags & PF_FAULT_CALL) &&
			--current->num_to_fault == 0;
}

long fail_syscall(void) {
	current->flags &= ~PF_FAULT_CALL;
	return -EINVAL;
}
