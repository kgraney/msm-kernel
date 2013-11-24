#include <linux/syscalls.h>
#include <linux/namei.h>
#include <asm-generic/uaccess.h>
#include <linux/sched.h>
#include <linux/fs_struct.h>
#include <linux/path.h>

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
	struct dentry *src_dent, *dest_dent;
	struct inode *src_inode, *dest_dir;
	struct path dest_path;
	int err;
	char buf[512], *s;

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

	dest_dent = kern_path_create(AT_FDCWD, dest, &dest_path, 0);
	if (IS_ERR(dest_dent)) {
		err = PTR_ERR(dest_dent);
		goto err;
	}

	dest_dir = dest_dent->d_parent->d_inode;
	if (!dest_dir) goto err;

	s = dentry_path_raw(dest_dent, buf, 512);
	printk(KERN_WARNING "COW: dest=%s -- %s", buf, s);
	s = dentry_path_raw(src_dent, buf, 512);
	printk(KERN_WARNING "COW: src=%s -- %s", buf, s);
	s = dentry_path_raw(dest_dent->d_parent, buf, 512);
	printk(KERN_WARNING "COW: dir=%s -- %s", buf, s);

	printk(KERN_WARNING "COW: path:%p dest_dent:%p", dest_path.dentry, dest_dent);
	s = dentry_path_raw(dest_path.dentry, buf, 512);
	printk(KERN_WARNING "COW: path=%s -- %s", buf, s);

	if (src_inode->i_sb != dest_dir->i_sb) {
		err = -EXDEV;
		goto err;
	}

	err = vfs_link(src_dent, dest_dir, dest_dent);
	if (err) {
		printk(KERN_WARNING "COW: error in vfs_link");
		goto err;
	}

	printk(KERN_DEBUG "COW: src_inode=%p dest_inode=%p", src_dent->d_inode, dest_dent->d_inode);
	path_put(&dest_path);
	dput(dest_dent);
	dput(src_dent);

	err = 1;
	src_inode->i_op->setxattr(src_dent, "system.ext4_cow", &err, 1, 0);

	return 0;

err:
	printk(KERN_DEBUG "COW: returning error (%d)", err);
	return err;
}
