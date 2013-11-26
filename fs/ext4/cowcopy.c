#include <linux/syscalls.h>
#include <sys/stat.h>
#include <errno.h>
//#include<blockdev.h> //ibdev
	
/**
 * src: path to the file to be copied
 * dest: path to the copy
 */
SYSCALL_DEFINE2(sys_ext4_cowcopy, const char __user*, src, const char __user*, dest)
{
	struct path src_path;
	struct path dst_path;
	struct inode* src_inode;
	struct inode* dst_inode
	struct file_system_type* src_fs;

	/* return error if path doesn't exist */
	if (user_path(src, src_path))
		return -EINVAL;
	
	src_inode = src_path.dentry->d_inode;

	/* return error if src is not a regular file */
/*	if (!S_ISREG(src_inode->i_mode))
		return -EPERM;
	
	src_fs = src_inode->i_sb->s_type;// src's file system type
*/
	/* src must be a file in ext4 file system, otherwise return -EOPNOTSUPP */
/*	if (src_fs.name = "ext4")
		return -EOPNOTSUPP;
*/	
	/* create a new file for dest */
/*	if (open(dest, O_CREAT, RDWR))
		return -1;//find erro code
	
	if (user_path(dest, dst_path))
		return -EINVAL;
	
	dst_inode = dst_path.dentry->d_inode;
*/
	/*src and dest should be in the same device, otherwise return -EXDEV*/
/*	if (I_BDEV(src_inode) == I_BEDV(dst_inode))
		return -EXDEV;
*/	
	/* start copying attributes now? */
//	printk(KERN_DEBUG "COWCOPY: sys_ext4_cowcopy");

	/* make a copy of src at dest */
	return 0;

}

