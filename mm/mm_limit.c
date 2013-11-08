#include <linux/sched.h>
#include <linux/syscalls.h>

SYSCALL_DEFINE2(set_mlimit, uid_t, uid, long, mem_max)
{
	struct user_struct *user = find_user(uid);
	if (!user)
		return -EINVAL;

	user->mm_limit = mem_max;
	return 0;
}
