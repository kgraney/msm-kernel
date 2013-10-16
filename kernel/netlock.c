 #include<mutex.h>
 #include<wait.h>

/* IS THIS IMPLEMENTATION CORRECT FOR ARM? */
/*
 * I think we need to implement this with native locks
 */
long test_and_set(volatile long* lock)
{
	int old = *lock;
	/*
	asm("xchgl %0, %1"
	    :"=r"(old), "+m"(*lock)	/* output */
	    :"0"(1)			/* input *
	    :"memory"			/* can clober anything in memory */
	    );
	 */
	spin_lock(&guard);
	*lock = 1;
	spin_unlock(&guard);
	return old;
}

/* sleeping lock structure */
struct mutex
{
	int flag; 		/* mutual exclusion status: 	0 -> available; 1 -> unavailable */
	int guard;		/* guard to avoid losing wakups:0 -> available; 1 -> unavailable */
	struct wait_queue_t q;	/* queue for waiting threads */
};

/* method to aquire mutual exclusion */
void p(mutex* m)
{
	while (test_and_set(m->guard)); 		/* spin while waiting for the guard */
	if (!(m->flag))
	{
		m->flag = 1; 				/* aquire mutex */
		m->guard = 0;				/* release guard lock */
	{
	else 
	{
		add_wait_queue(&m->q, %current->wait);	/* add task to wait queue */
		m->guard = 0;				/* release guard lock */
		schedule();				/* schedule process to sleep */
	}
}

/* method to relinquish mutual exclusion */
void v(mutex* m)
{
	while(test_and_set(m->guard)); 					/* spin while waiting for the guard */
	if (list_empty(&m->q))						/* nothing waiting for lock */
		m->flag = 0;						/* release mutex */
	else								/* directly transfer mutex to next thread */
		wake_up(remove_waitqueue(&m->q, &current->wait));	/* this my be problematic in providing priority to writers? */
	m->guard = 0;
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
	
/* netlock initialization routine */
void netlock_init(void)
{
	/* declare spinlock for test_and_set routine */
	DEFINE_SPINLOCK(guard);
	
	/* declare sleeping locks for network access */
	static mutex* mutex1;
	static mutex* mutex2;
	static mutex* mutex3;
	static mutex* w;
	static mutex* r;
	
	/* initially, no task has requested a netlock */
	mutex1->flag = 0;
	mutex2->flag = 0;
	mutex3->flag = 0;
	w->flag = 0;
	r->flag = 0;
	
	mutex1->guard = 0;
	mutex2->guard = 0;
	mutex3->guard = 0;
	w->guard = 0;
	r->guard = 0;

	/* intially, no task has a netlock */
	current->w_count = 0;
	current->r_count = 0;

	/* initialize sleeping lock wait queues */
	init_waitqueue_head(&mutex1->q);
	init_waitqueue_head(&mutex2->q);
	init_waitqueue_head(&mutex3->q);
	init_waitqueue_head(&w->q);
	init_waitqueue_head(&r->q);

	init_waitqueue_entry(&current->wait);
}

SYSCALL_DEFINE1(netlock_aquire, netlock_t, type)
{	
	switch (type)	
	case NET_LOCK_E:
		//exclusive_netlock();
		p(mutex2);				/* aquire exclusive access to write count */
			current->w_count++; 		/* increment write count */
			if (current->w_count == 1)	/* exlcusive network acess */
				p(r);			/* exclude readers from network access */
		v(mutex2);				/* release lock to write count */
		p(w);					/* aquire exclusive network access */
		return 0;// writing ensues 
	case NET_LOCK_R:
		//read_netlock
		p(mutex3);						/* aquire exclusive access to read lock */
			p(r);						/* aquire access to read network */
				p(mutex1);				/* aquire exclusive access to read count */
					current->r_count++;		/* increment read count */
					if (current->r_count == 1)	/* if this is the first reader... */
						p(w);			/* block writers from network access */
				v(mutex1);				/* release lock to read count */
			v(r);						/* release lock to read network? */
		v(mutex3);						/* release lock to read lock */
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
		return 0;/* not sure what to do with this */
	default:
		return -1;	
}
