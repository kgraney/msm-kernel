 #include<mutex.h>
	
int p(mutex* m)/* down */ 
{
	return meutex_lock(m);
}
	
int v(mutex* m)/* up */
{
	return mutex_unlock(m);
}

/* system call wrappers */
int netlock_aquire(netlock_t type)
{
	return sys_netlock_aquire(type);
}
int netlock_release(void)
{
	return sys_netlock_release();
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

	SYSCALL_DEFINE1(netlock_aquire, netlock_t, type)
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

	SYSCALL_DEFINE0(netlock_release)	
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
