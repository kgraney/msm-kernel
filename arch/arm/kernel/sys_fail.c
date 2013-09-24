/*
 *  linux/arch/arm/kernel/sys_fail.c
 *
 *  This file contains a system call to book keep information about the
 *  current process.
 */
#include <linux/sched.h>
#include <linux/current.h>

/*
 * Inject a system failure fault 
 */
asmlinkage long sys_fail(int N)
{
	// get current process state
	struct task_struct* task = get_current();
	task->sys_calls_left--;
	//
}

// should i define a c file that calls this system call?


/* 
 * Bookkeep process information
 *
int fail(int N) // userspace call
{
	struct task_struct* task;
	task = get_current();

	
	if (N > 0)
		// return error code of N-th system call
	if (N < 0)
		//return error code
		return -EINVAL;
	else
	{
		if (//fault injection running)
		//stop fault injection session, if they're already going on
		else 
		// return error code
	}
	
	return 0;
}
*/

EXPORT_SYMBOL(sys_fail);
