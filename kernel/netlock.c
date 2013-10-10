/*
 You may assume that all processes that intend to use the network must call netlock_acquire in regular (read) mode. The calls to acquire the lock in regular mode should succeed immediately as long as no process is holding an exclusive (write) lock or is waiting for an exclusive lock. Multiple processes may request the lock in regular mode, and all of them should be granted the lock if this condition is met. If a process is currently holding or waiting for an exclusive lock, subsequent calls to netlock_acquire in regular mode must block. All processes waiting for regular locks must be granted the lock once the condition for granting the regular locks is fulfilled.

Only one process may hold an exclusive lock at any given time. When a process requests an exclusive lock, it must wait until processes currently holding regular or exclusive locks release the locks using the netlock_release system call. Processes requesting an exclusive lock get priority over processes waiting to get regular locks. If multiple processes request exclusive locks, they should be granted locks in the order of their request.
*/

// how do i know someone is waiting for a write lock? --> two semaphores: one binary for exclusive, one counting for read
// how do i know someone is waiting for a read lock? netlock_r->count > 0 when someone hold the lock

 #include<mutex.h>
/* 
 * Syscall 378. Acquire netlock. type indicates whether a regular or exclusive lock is needed. Returns 0 on success and -1 on failure.
 */
	int p(mutex* m)/* down */ 
	{
		mutex_lock(m);
	}
	
	int v(mutex* m)/* up */
	{
		mutex_unlock(m);
	}
	
	
	/*
	 * the mutex declarations need to be intialized some way...
	 */
	//unsigned long readcount;	/* initialize to 0 */ //defined in task_struct...don't know how to initialize this way
	//unsigned long writecount;	/* initialize to 0 */ //defined in task_struct
	static DECLARE_MUTEX(mutex1);	/* initialize to 1 */
	static DECLARE_MUTEX(mutex2);	/* initialize to 1 */
	static DECLARE_MUTEX(mutex3);	/* initialize to 1 */
	static DECLARE_MUTEX(w);	/* initialize to 1 */
	static DECLARE_MUTEX(r);	/* initialize to 1 */
	  
	 //sema_init(&netlock_r, SEMMSL);//allow count to max number of semaphores of process

	int netlock_acquire(netlock_t type)
	{	
		switch (type)	
		case NET_LOCK_E:
			p(mutex2);
				current->w_count++;
				if (current->w_count == 1)
					p(r);
			v(mutex2);
			p(w);
			return 0;// writing ensues 
		case NET_LOCK_R:
			p(mutex3);
				p(r);
					p(mutex1);
						current->r_count++;
						if (current->r_count == 1)
							p(w);
					v(mutex1);
				v(r);
			v(mutex3);
			return 0;// reading ensues
		case NET_LOCK_N:
			return 0;/* not sure what to do with this */
		default:
			return -1;
	}

	 /* Syscall 379. Release netlock. Return 0 on success and -1 on failure.  
	  */
	int netlock_release(void)
	{
		switch (current->netlock_type)
		case w:
			v(w);
			p(mutex2);
				current->w_count--;
				if (current->w_count == 0)
					v(r);
			v(mutex2);
			return 0;
		case r:
			p(mutex1);
				current->r_count--;
				if (current->r_count == 0)
					v(w);
			v(mutex1);
			return 0;
		case n:
			return 0;
		default:
			return -1;	
	}

	enum __netlock_t 
	{
		NET_LOCK_N, /* Placeholder for no lock */
		NET_LOCK_R, /* Indicates regular lock */
		NET_LOCK_E, /* Indicates exclusive lock */
	};
	typedef enum __netlock_t netlock_t;
