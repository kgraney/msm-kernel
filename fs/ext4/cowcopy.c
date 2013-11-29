#include <linux/syscalls.h>
#include <linux/namei.h>
#include <asm-generic/uaccess.h>
#include <linux/sched.h>
#include <linux/fs_struct.h>
#include <linux/path.h>
#include <linux/xattr.h>

static struct dentry* __get_dentry(const char *path_str, int flags, int *err)
{
	struct path path, *relative = &current->fs->pwd;

	*err = vfs_path_lookup(relative->dentry, relative->mnt, path_str,
			flags | LOOKUP_FOLLOW | LOOKUP_AUTOMOUNT, &path);
	if (unlikely(*err))
		return 0;

	dget(path.dentry);
	return path.dentry;
}

SYSCALL_DEFINE2(ext4_cowcopy, const char __user, *src, const char __user, *dest)
{
	struct dentry *src_dent;
	struct inode *src_inode;
	char xattr_val;
	int err;

	if (!access_ok(VERIFY_READ, src, 1) || !access_ok(VERIFY_READ, dest, 1))
		return -EINVAL;

	printk(KERN_WARNING "COW: copying %s to %s", src, dest);

	src_dent = __get_dentry(src, 0, &err);
	if (!src_dent) goto err;
	src_inode = src_dent->d_inode;
	if (!src_inode) goto err;

	if (S_ISDIR(src_inode->i_mode) || !S_ISREG(src_inode->i_mode))
		return -EPERM;

	/* TODO: check fs type of src_inode is ext4 */
	if (strcmp(src_inode->i_sb->s_type->name, "ext4"))
		return -EOPNOTSUPP;

	err = sys_linkat(AT_FDCWD, src, AT_FDCWD, dest, 0);
	if (err) goto err;

	xattr_val = 1;
	err = vfs_setxattr(src_dent, "user.ext4_cow", &xattr_val, sizeof(char), 0);
	if (err) {
		printk(KERN_WARNING "COW: setxattr error");
		goto err;
	}

	/*
	xattr_val = 100;
	err = vfs_getxattr(src_dent, "user.ext4_cow", &xattr_val, sizeof(char));
	if (err != sizeof(char)) {
		printk(KERN_WARNING "COW: getxattr error");
		goto err;
	}
	printk(KERN_WARNING "COW: xattr = %d", xattr_val);
	*/

	return 0;

err:
	printk(KERN_DEBUG "COW: returning error (%d)", err);
	return err;
}
