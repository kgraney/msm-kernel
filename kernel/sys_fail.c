#include <linux/syscalls.h>
#include <linux/errno.h>

SYSCALL_DEFINE1(fail, int, n)
//asmlinkage int sys_fail(int n)
{
  current->fault_countdown--;
  if(n > 0)
  {
    current->fault_countdown = n;
    current->flags |= PF_FAULT_INJECTION;
    return 0;
  }
  else if((n == 0) && (current->flags & PF_FAULT_INJECTION))
  {
    current->flags &= ~PF_FAULT_INJECTION;
    return 0;
  }
  else
    return -EINVAL;
}
long should_fail(void) {
  current->fault_countdown--;
  return ((current->flags & PF_FAULT_INJECTION) &&
      (current->fault_countdown == 0));  /* predecrement for this system call?  */
}
long fail_syscall(void) {
  current->flags &= ~PF_FAULT_INJECTION;  /* clear after this failure */
  return -EINVAL;
}
