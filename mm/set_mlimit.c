#include <linux/syscalls.h>

/*** 
 * system call to set the maximum amount of physical memory a specified user
 * can allocate
 */
SYSCALL_DEFINE2(set_mlimit, uid_t, uid, long, mem_max)
{
	struct user_struct* user = find_user(uid);
	printk(KERN_DEBUG "SET_MLIMIT: set_mlimit");

	/* user can't be found */
	if (!user)
		return -1;
	
	/* set the maxium amount of memory the user may allocate */
	user->mem_max = mem_max;
	printk(KERN_DEBUG "SET_MLIMIT: mem_max=%ld", user->mem_max);
	return 0;
}

