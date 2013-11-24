#include <linux/syscalls.h>
#include <linux/namei.h>
#include <asm-generic/uaccess.h>
#include <linux/sched.h>
#include <linux/fs_struct.h>
#include <linux/path.h>

static struct inode* __get_inode(const char *path_str, int *err)
{
	struct path path, *relative = &current->fs->pwd;

	*err = vfs_path_lookup(relative->dentry, relative->mnt, path_str,
			0, &path);
	if (unlikely(*err))
		return 0;

	return path.dentry->d_inode;
}

SYSCALL_DEFINE2(ext4_cowcopy, const char __user, *src, const char __user, *dest)
{
	struct inode *src_inode;
	int err;

	if (!access_ok(VERIFY_READ, src, 1) || !access_ok(VERIFY_READ, dest, 1))
		return -EINVAL;

	printk(KERN_WARNING "COW: copying %s to %s", src, dest);

	src_inode = __get_inode(src, &err);
	if (!src_inode) goto err;

	if (S_ISDIR(src_inode->i_mode) || !S_ISREG(src_inode->i_mode))
		return -EPERM;

	return -1; /* TODO */

err:
	printk(KERN_DEBUG "COW: returning error (%d)", err);
	return err;
}
