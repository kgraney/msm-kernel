/*
 *  linux/arch/arm/kernel/sys_fail.c
 *
 *  This file contains a system call to book keep information about the
 *  current process.
 */
#include <linux/sched.h>
#include <linux/current.h>
#include<sys/sys_call.h>//double check path
#include<linux/errno.h>//check path

/*
 * Inject a system failure fault
 */
asmlinkage long sys_fail(int N)
{
	// get current process
	struct task_struct* task = get_current();

	// check input
	if (N < 0 || (N == 0 && task->fault_session_active == 0))
		return -EINVAL;// invalid input

	// target system call reached
	if (N == 0 && task->fault_session_active == 1)
		return -EINJECT;// inject fault

	// update bookkeeping information
	task->sys_calls_left = N;
	task->fault_session_active = 1;
	return 0;
}


/*
 * This routine returns 1 if current system call should be failed, and 
 * otherwise returns 0.
 */
long should_fail(void)
{
	// get current process
	struct task_struct* task = get_current();

	// fail system call when counter zeros
	if (task->sys_calls_left == 0)
		return 1;
	// decrement and pass
	task->sys_calls_left--
		return 0;
}

/*
 * This routine returns 1 if the system call failed, and otherwise 
 * returns 0.
 */
long fail_syscall(void)
{
	if (should_fail() == 1)
		return -EINJECT;
	return 0;
}

EXPORT_SYMBOL(sys_fail);
