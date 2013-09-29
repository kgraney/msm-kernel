/*
 *  linux/arch/arm/kernel/sys_fail.c
 *
 *  This file contains a system call to book keep information about the
 *  current process.
 */
#include<linux/syscalls.h>//double check path
#include<linux/errno.h>//check path

/*
 * Inject a system failure fault
 */

// kevin's code
/*
SYSCALL_DEFINE1(fail, int, n)
{
  if (n > 0) {
    // fail the nth system call
    current->num_to_fault = n;
    current->flags |= PF_FAULT_CALL;
    return 0;

  } else if (n == 0 && (current->flags & PF_FAULT_CALL)) {
    // stop fault injection session
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
*/


// dainis's code
/*
asmlinkage long sys_fail(int N)
{
	current->fault_countdown--;

	if (N>0)
	{
		current->fault_countdown = N;
		current->flags |= PF_FAULT_INJECTION;
		return 0;
	}

	else if (N == 0 && (current->flags & PF_FAULT_INJECTION))
	{
		current->flags &=~PF_FAULT_INJECTION;
		return 0;
	}
	else 
		return -EINVAL;
}
*/

// my code
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


/***********************************************************************
 * This routine returns 1 if current system call should be failed, and 
 * otherwise returns 0.
 ***********************************************************************/

// dainis's code
/* 
long should_fail(void)
{
	current->fault_countdown--;
	return ((current->flags & PF_FAULT_INJECTION) && (current->fault_countdown == 0));
}
*/


// my code
long should_fail(void)
{
	// fail system call when counter zeros
	if (current->sys_calls_left == 0 && (current->fault_session_active == 1))
		return 1;
	// decrement and pass
	current->sys_calls_left--;
		return 0;
}



/********************************************************************
 * This routine returns 1 if the system call failed, and otherwise 
 * returns 0.
 *
 * currently, kevin nor dainis's code does this; they only return -1
 ********************************************************************/

// dainis's code
/* 
long fail_syscall(void)
{

	current->flags &= ~PF_FAULT_INJECTION;
	return -EINVAL;
}
*/

//my code
/* 
 * terminate the fault injection session and return an error code
 */
long fail_syscall(void)
{
	current->fault_session_active = 0;
	return -EINJECT;
}





