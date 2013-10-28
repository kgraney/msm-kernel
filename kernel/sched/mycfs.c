/* my completely fair scheudler class */

/* 
 * DESIGN STEPS
 *
 * - test all the primative functions to ensure i know how they work
 * - implement scheduler in peices and debug
 * - write a user program to test perhaps?
 *
 */

/*
 * seperate queue for each cpu
 */


#include "sched.h"

/* these hooks print debug data. they should be populated as development evolves */
static void enqueue_task_mycfs(struct rq* rq, struct task_struct* p, int flags)
{
	printk(KERN_DEBUG "mycfs: enqueue_task_mycfs");
}

static void dequeue_task_mycfs(struct rq* rq, struct task_struct* p, int flags)
{
	printk(KERN_DEBUG "mycfs: dequeue_task_mycfs");
	raw_spin_unlock_irq(&rq->lock);
	dump_stack();
	raw_spin_lock_irq(&rq->lock);
}

static void check_preempt_curr_mycfs(struct rq* rq, struct task_struct* p, int flags)
{
	printk(KERN_DEBUG "mycfs: check_preempt_curr");
	resched_task(rq->idle);
}

static struct task_struct* pick_next_task_mycfs(struct rq* rq)
{
	printk(KERN_DEBUG "mycfs: pick_next_task_mycfs");
	schedstat_inc(rq, sched_goidle);
	calc_load_account_idle(rq);
	return rq->idle;
}

static void put_prev_task_mycfs(struct rq* rq, struct task_struct* p)
{
	printk(KERN_DEBUG "mycfs: put_prev_task_mycfs");
}

#ifdef CONFIG_SMP
static int select_task_rq_mycfs(struct task_struct* p, int sd_flag, int flags)
{
	printk(KERN_DEBUG "mycfs: select_task_rq_mycfs");
	return task_cpu(p);;
}
#endif

static void set_curr_task_mycfs(struct rq* rq)
{
	printk(KERN_DEBUG "mycfs: set_curr_task_mycfs");
}

static void task_tick_mycfs(struct rq* rq, struct task_struct* p, int queued)
{
	printk(KERN_DEBUG "mycfs: task_tick_mycfs");
}

static unsigned int get_rr_interval_mycfs(struct rq* rq, struct task_struct* p)
{
	printk(KERN_DEBUG "mycfs: get_rr_interval_mycfs");
	return 0;
}

static void prio_changed_mycfs(struct rq* rq, struct task_struct* p, int oldprio)
{
	printk(KERN_DEBUG "mycfs: prio_changed_my_cfs");
}

static void switched_to_mycfs(struct rq* rq, struct task_struct* p)
{
	printk(KERN_DEBUG "mycfs: switched_to_mycfs");
}

/* declaration of the mycfs class */
const struct sched_class mycfs_class = 
{
	/* hooks */
	.enqueue_task 		= enqueue_task_mycfs,
	.dequeue_task 		= dequeue_task_mycfs,

	.check_preempt_curr 	= check_preempt_curr_mycfs,
	.pick_next_task 	= pick_next_task_mycfs,
	.put_prev_task 		= put_prev_task_mycfs,
#ifdef CONFIG_SMP
	.select_task_rq 	= select_task_rq_mycfs,
#endif

	.set_curr_task 		= set_curr_task_mycfs,
	.task_tick 		= task_tick_mycfs,

	.get_rr_interval 	= get_rr_interval_mycfs,
	
	.prio_changed 		= prio_changed_mycfs,
	.switched_to 		= switched_to_mycfs,
};

/* initialization routine for mycfs classes */
__init void init_sched_mycfs_class(void)
{
	printk(KERN_DEBUG "mycfs_class: init_sched_mycfs_class");
}


