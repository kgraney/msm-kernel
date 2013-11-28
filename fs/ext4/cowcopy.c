#include <linux/syscalls.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/namei.h>
#include <linux/fs.h>

SYSCALL_DEFINE2(ext4_cowcopy, const char __user, *src, const char __user, *dest)
{
	struct path src_path;
	struct dentry *src_dentry;
	struct inode *src_inode;
	struct path dest_path;
	struct dentry *dest_dentry;
	struct inode *dest_inode;
	struct file_system_type *src_fs_type;
	struct block_device *src_device;
	struct block_device *dest_device;
	int rc;

	printk(KERN_DEBUG "COWCOPY: copy on write system call");
	rc = user_path(src, &src_path);
	if(unlikely(rc))
	{
		printk(KERN_DEBUG "COWCOPY: src path is invalid");
 		return -EINVAL;
	}
	rc = user_path(dest, &dest_path);
	if(unlikely(rc))
	{
		printk(KERN_DEBUG "COWCOPY: dest path is invalid");
 		return -EINVAL;
	}
	src_dentry = src_path.dentry;
	src_inode = src_dentry->d_inode;
	if (!S_ISREG(src_inode->i_mode))
	{
		printk(KERN_DEBUG "COWCOPY: src is not a regular file");
 		return -EPERM;
	}
 	src_fs_type = src_inode->i_sb->s_type;
	if(strcmp(src_fs_type->name, "ext4"))
	{
		printk(KERN_DEBUG "COWCOPY: src is not an ext4 file");
 		return -EOPNOTSUPP;
	}
	dest_dentry = dest_path.dentry;
	dest_inode = dest_dentry->d_inode;
	src_device = src_inode->i_sb->s_bdev;
	dest_device = dest_inode->i_sb->s_bdev;
	if(src_device != dest_device)
	{
		printk(KERN_DEBUG "COWCOPY: src and dest are not on the same device");
 		return -EXDEV;
	}
	return 0;
}
