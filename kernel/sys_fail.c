/*
 *  linux/arch/arm/kernel/sys_fail.c
 */

#include<linux/syscalls.h>//double check path
#include<linux/errno.h>//check path

/*********************************************************************
 * System call to inject system call faults N calls from the injection
 * call (i.e. when sys_fail() is called). 
 ********************************************************************/
asmlinkage long sys_fail(int N)
{
	// check input
	if (N < 0 || (N == 0 && current->fault_session_active == 0))
		return -EINVAL;// invalid input

	// target system call reached
	if (N == 0 && current->fault_session_active == 1)
		return -EINJECT;// inject fault

	// update bookkeeping information
	current->sys_calls_left = N;
	current->fault_session_active = 1;
	return 0;
}

/*********************************************************************
 * This system call routine returns 1 if current system call should be
 * failed, and otherwise returns 0, after bookkeeping the number of 
 * calls to target attribute (i.e. counter).
 ********************************************************************/
long should_fail(void)
{
	// fail system call when counter zeros
	if (current->sys_calls_left == 0 && (current->fault_session_active == 1))
		return 1;
	// decrement and pass
	current->sys_calls_left--;
		return 0;
}

/*********************************************************************
 * This system call routine returns an artificial error code, indicat-
 * ing a system call has failed, after terminating the fault injection
 * session.
 ********************************************************************/
long fail_syscall(void)
{
	current->fault_session_active = 0;// terminate 
	return -EINJECT;// fail
}
