#include <linux/syscalls.h>
#include <linux/namei.h>
#include <asm-generic/uaccess.h>
#include <linux/sched.h>
#include <linux/fs_struct.h>
#include <linux/path.h>
#include <linux/xattr.h>

struct dentry* __get_dentry(const char *path_str, int flags, int *err)
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
	int err = -EINVAL;
	char *ksrc, *kdest;

	ksrc = getname(src);
	kdest = getname(dest);

	printk(KERN_WARNING "COW: copying %s to %s", ksrc, kdest);

	src_dent = __get_dentry(ksrc, 0, &err);
	if (!src_dent) goto err;
	src_inode = src_dent->d_inode;
	if (!src_inode) goto err;

	if (S_ISDIR(src_inode->i_mode) || !S_ISREG(src_inode->i_mode)) {
		err = -EPERM;
		goto err;
	}

	if (atomic_read(&src_inode->i_writecount) > 0) {
		err = -EPERM;
		goto err;
	}

	if (strcmp(src_inode->i_sb->s_type->name, "ext4")) {
		err = -EOPNOTSUPP;
		goto err;
	}

	err = sys_linkat(AT_FDCWD, src, AT_FDCWD, dest, 0);
	if (err) goto err;

	xattr_val = 1;
	err = vfs_setxattr(src_dent, "user.ext4_cow", &xattr_val, sizeof(char), 0);
	if (err) {
		printk(KERN_WARNING "COW: setxattr error");
		goto err;
	}

	return 0;
err:
	putname(kdest);
	putname(ksrc);

	printk(KERN_DEBUG "COW: returning (%d)", err);
	return err;
}
