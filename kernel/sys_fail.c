/*
 *  linux/arch/arm/kernel/sys_fail.c
 */
#include <linux/syscalls.h>
#include <linux/errno.h>

/*********************************************************************
 * System call to inject system call faults N calls from the injection
 * call (i.e. when sys_fail() is called).
 ********************************************************************/
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

/*********************************************************************
 * This system call routine returns 1 if current system call should be
 * failed, and otherwise returns 0, after bookkeeping the number of
 * calls to target attribute (i.e. counter).
 ********************************************************************/
long should_fail(void) {
	return (current->flags & PF_FAULT_CALL) &&
			--current->num_to_fault == 0;
}

/*********************************************************************
 * This system call routine returns an artificial error code, indicat-
 * ing a system call has failed, after terminating the fault injection
 * session.
 ********************************************************************/
long fail_syscall(void) {
	current->flags &= ~PF_FAULT_CALL;
	return -EINVAL;
}
