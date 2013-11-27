#include <linux/syscalls.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/namei.h>
#include <linux/fs.h>
#include <linux/blockdev.h> //ibdev
	
/**
 * src: path to the file to be copied
 * dest: path to the copy
 */
SYSCALL_DEFINE2(sys_ext4_cowcopy, const char __user*, src, const char __user*, dest)
{
	struct path src_path;
	struct path dst_path;
	struct inode* src_inode;
	struct inode* dst_inode;
	const char* src_fs_name;

	printk(KERN_DEBUG "COW: sys_ext4_cowcopy");

	/* return error if path doesn't exist */
	if (user_path(src, &src_path))
	{
		printk(KERN_WARNING "COW: path doesn't exist");
		return -EINVAL;
	}
	
	src_inode = src_path.dentry->d_inode;

	/* return error if src is not a regular file */
	if (!S_ISREG(src_inode->i_mode))
	{
		printk(KERN_WARNING "COW: src is not a regular file");
		return -EPERM;
	}
	
	src_fs_name = src_inode->i_sb->s_type->name;// src's file system type

	/* src must be a file in ext4 file system, otherwise return -EOPNOTSUPP */
	if (strcmp(src_fs_name, "ext4"))
	{
		printk(KERN_WARNING "COW: src is not a ext4 file");
		return -EOPNOTSUPP;
	}

/* i should consider more efficient means of copying the file's content -- link perhaps */
	/* create a new file for dest */
/*	if (open(dest, O_CREAT, RDWR))
	{
		printk(KERN_WARNING "COW: failed to create a new file");
		return -1;//find erro code
	}
*/	
	if (user_path(dest, &dst_path))//look at values of dst_path...
	{
		printk(KERN_WARNING "COW: dst_path is not valid");
		return -EINVAL;
	}
	
	dst_inode = dst_path.dentry->d_inode;

	/* src and dest should be in the same device, otherwise return -EXDEV */
	if (I_BDEV(src_inode) == I_BEDV(dst_inode))
	{
		printk(KERN_WARNING "COW: src and dest have mis-matching devices");
		return -EXDEV;
	}
	
	/* start copying attributes now? */
//	printk(KERN_DEBUG "COWCOPY: sys_ext4_cowcopy");

	/* make a copy of src at dest */
	return 0;
}

