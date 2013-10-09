#include<sthread.h>
/*
 * netlock.c for homework 3
 */

/*
 * Syscall 378. Acquire netlock. type indicates
 * whether a regular or exclusive lock is needed. Returns 0 on success 
 * and -1 on failure.  
 */
int netlock_acquire(netlock_t type)
{
	// define netlock request queue
	DECLARE_WAITQUEUE(reqq);
	
	switch(type)
	{
	case NET_LOCK_E:/* exclusive lock */
		// define queue entry for the exclusive lock request
		DEFINEWAIT(req);

		// add to netlock request queue
		add_to_wait(&reqq, &req);

		while (!lock_avail)
		{
			prepare_to_wait(&nlq, &req, TASK_INTERRUPTIBLE);
			if (signal_pending(current))
				/* handle signal */
			schedule();
		}
		finish_wait(&nlq, &req);//lock network	

		// block if other processes have an exclusive lock --> how do i block?
		// only allow one of these locks to be held, otherwise queue the requests
		return 0;
	case NET_LOCK_R: /* normal lock */
		// grant lock as long as no other process has (or is waiting for) an exclusive lock
		// allow multiple locks in this mode, provided the above conditions are satisfied
		return 0;
	case NET_LOCK_N:	/* no lock -> not sure how this is used yet*/
		return 0;
	default:		/* error */
		return -1;
	}
}
/* 
 * Syscall 379. Release netlock. Return 0 on success and -1 on failure.  
 */
int netlock_release(void);
{
	// unlock
	// remove from queues
{

enum __netlock_t 
{
	NET_LOCK_N, /* Placeholder for no lock */
	NET_LOCK_R, /* Indicates regular lock */
	NET_LOCK_E, /* Indicates exclusive lock */
};
typedef enum __netlock_t netlock_t;
