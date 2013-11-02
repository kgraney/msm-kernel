/* my completely fair scheudler class */

/* 
 * DESIGN STEPS
 *
 * - just schedule one process to start
 * - add yield and fork functionality later
 * - add priority
 * - sched latency is 10ms
 * - utilize all cores with a run queue for each core
 * - 
 */

#include "sched.h"  

#define for_each_sched_entity(se) \
		for(; se; se = se->parent)

/* insert a schedule entity into mycfs rb_tree */
static void __enqueue_entity(struct mycfs_rq* mycfs_rq, struct sched_entity* se)
{
	struct rb_node** link = &mycfs_rq->mycfs_root.rb_node;		/* mycfs root */
	struct rb_node* parent = NULL;					/* no parent to root */
	struct sched_entity* entry;					/* sched entity */
	int leftmost = 1;						

	while(*link)							/* loop until leaf is found */
	{
		parent = *link; 					/* parent is the old leaf */
		entry = rb_entry(parent, struct sched_entity, run_node);/* insert entity in tree */
		if (se->vruntime < entry->vruntime)			/* vruntime is smaller than parent */		
			link = &parent->rb_left;			/* place link to the left of parent */
		else							/* vruntime is larger than parent */
		{
			link = &parent->rb_right;			/* place link to the right of the parent */
			leftmost = 0;					/* this is not the left-most node */
		}
	}

	if (leftmost)							/* cache the left-most node */
		mycfs_rq->rb_leftmost = &se->run_node;

	rb_link_node(&se->run_node, parent, link);			/* inserted process is the new child */
	rb_insert_color(&se->run_node, &mycfs_rq->mycfs_root);		/* update the self-balancing properites */
}

/* removes a schedule entity from the rb tree */
static void __dequeue_entity(struct mycfs_rq* mycfs_rq, struct sched_entity* se)
{
	if (&se->run_node == mycfs_rq->rb_leftmost)			/* the dequeued node is the left-most node */
	{	
		struct rb_node* prev_node;
		prev_node = rb_next(&se->run_node);			/* the node before the dequeued node */
		mycfs_rq->rb_leftmost = prev_node;			/* previous node becomes the left-most node */
	}
	
	rb_erase(&se->run_node, &mycfs_rq->mycfs_root);			/* erase the sched_entity from the tree */
}

static inline u64 max_vruntime(u64 min_vruntime, u64 vruntime)
{
	s64 delta = (s64)(vruntime - min_vruntime);

	if (delta > 0)		/* vruntime is larger */
		return vruntime;
	
	return min_vruntime;	/* min_vruntime is larger */
}

static inline u64 min_vruntime(u64 min_vruntime, u64 vruntime)
{
	s64 delta = (s64)(vruntime - min_vruntime);
	
	if (delta < 0)		/* vruntime is smaller */
		return vruntime;
	
	return min_vruntime;	/* min_vruntime is smaller */
}

static void update_min_vruntime(struct mycfs_rq* mycfs_rq)
{
	u64 vruntime = mycfs_rq->min_vruntime;

	/* current task */
	if (mycfs_rq->curr)
		vruntime = mycfs_rq->curr->vruntime;
	
	/* check if it's the left-most */
	if (mycfs_rq->rb_leftmost)
	{
		struct sched_entity* se = rb_entry(mycfs_rq->rb_leftmost, struct sched_entity, run_node);

		/* set the minimum vruntime to the left-most node */
		if(!mycfs_rq->curr)
			vruntime = se->vruntime;
		else
			vruntime = min_vruntime(vruntime, se->vruntime);
	}

	mycfs_rq->min_vruntime = max_vruntime(mycfs_rq->min_vruntime, vruntime);
}

static inline void __update_curr(struct mycfs_rq* mycfs_rq, struct sched_entity* se, unsigned long delta_exec)
{
	unsigned long delta_exec_weighted;
	
	schedstat_set(se->statistics.exec_max, max((u64)delta_exec, statistics.exec_max));
	
	se->sum_exec_runtime += delta_exec;
	schedstat_add(mycfs_rq, exec_clock, delta_exec);
	/* time differential is not weighted yet */
	delta_exec_weighted = delta_exec;

	se->vruntime += delta_exec_weighted;
	update_min_vruntime(mycfs_rq);
}

/* update vruntime */
static void update_curr(struct mycfs_rq* mycfs_rq)
{
	/* determine time since last call to update_curr()
	 * apply time weight and add to current task's
	 * vruntime
	 */
	struct sched_entity* curr = mycfs_rq->curr;
	u64 now = (mycfs_rq->rq)->clock_task;
	unsigned long delta_exec;
	
	delta_exec = (unsigned long)(now - curr->exec_start);
	
	/* return if calculation fails */
	if(!delta_exec)
		return;	

	__update_curr(mycfs_rq, curr, delta_exec);
	curr-exec_start = now;	/* current task starts execution now */
}

struct sched_entity* __pick_first_mycfs_entity(struct mycfs_rq* mycfs_rq)
{
	struct rb_node* left = mycfs_rq->rb_leftmost;

	if (!left)
		return NULL;
	
	return rb_entry(left, struct sched_entity, run_node);
}

/* preempt task if needed */
static void check_preempt_tick(struct mycfs_rq* mycfs_rq, struct sched_entity* curr)
{
	unsigned long ideal_runtime, delta_exec;
	struct sched_entity* se;
	s64 delta;

	ideal_runtime = 10000000;//10ms
	delta_exec = curr->sum_exec_runtime - curr->prev_sum_exec_runtime;

	if (delta_exec > ideal_runtime)
	{
		resched_task(mycfs_rq->rq->curr);
		return;
	}

	if (delta_exec < sysctl_sched_min_granularity)
		return;
	
	se = __pick_first_mycfs_entity(mycfs_rq);
	delta = curr->vruntime - se->vruntime;

	if (delta < 0)
		return;
	
	if (delta > ideal_runtime)
		resched_task(mycfs_rq->rq->curr);

}


/*  */
static void entity_tick(struct mycfs_rq* mycfs_rq, struct sched_entity* se, int dequeued)
{
	/* update statistics of the current task */
	update_curr(mycfs_rq);

	/* preempt when there's more than one task */
	if(mycfs_rq->nr_running > 1)
		check_preempt_tick(mycfs_rq, se);//should preempt after 10ms
}


/* update the scheduling stats, then insert task into rb tree */
static void enqueue_task_mycfs(struct rq* rq, struct task_struct* p, int flags)
{
	struct mycfs_rq* mycfs_rq = &rq->mycfs;

	printk(KERN_DEBUG "mycfs: enqueue_task_mycfs");
	__enqueue_entity(mycfs_rq, &p->se);

	inc_nr_running(rq); /* increment the cpu run queue */
}

/* dequeue a task from the scheduler queue */
static void dequeue_task_mycfs(struct rq* rq, struct task_struct* p, int flags)
{
	printk(KERN_DEBUG "mycfs: dequeue_task_mycfs");
	
	__dequeue_entity(&rq->mycfs, &p->se);

	dec_nr_running(rq);
}

/* checks if the new task should preempt the current task */
static void check_preempt_curr_mycfs(struct rq* rq, struct task_struct* p, int flags)
{
	printk(KERN_DEBUG "mycfs: check_preempt_curr");
	/* when would a task preempt the current task? */
}

/* return the left-most node from the tree -- otherwise return null */
static struct sched_entity* __pick_next_entity(struct mycfs_rq* mycfs_rq)
{
	struct rb_node* left = mycfs_rq->rb_leftmost;		/* get the left-most node */

	if (!left)						/* root is the left-most node */
		return NULL;				
	return rb_entry(left, struct sched_entity, run_node);	/* return the sched_entity there */
}

/* choose task with smallest vruntime, or left-most node in the rb_tree */
static struct task_struct* pick_next_task_mycfs(struct rq* rq)
{
	//struct task_struct* p;
	struct sched_entity* se;
	struct mycfs_rq* mycfs_rq = &rq->mycfs;

	printk(KERN_DEBUG "mycfs: pick_next_task_mycfs");
	
	se = __pick_next_entity(mycfs_rq);
	
	return container_of(se, struct task_struct, se);
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

/* called when a task changes scheduling classes */
static void set_curr_task_mycfs(struct rq* rq)
{
	printk(KERN_DEBUG "mycfs: set_curr_task_mycfs");
}

/* this may lead to process switch */
static void task_tick_mycfs(struct rq* rq, struct task_struct* p, int queued)
{
	struct mycfs_rq* mycfs_rq;
	struct sched_entity* se = &p->se;

	printk(KERN_DEBUG "mycfs: task_tick_mycfs");

	for_each_sched_entity(se)
	{
		mycfs_rq = se->mycfs_rq;
		entity_tick(mycfs_rq, se, queued);
	}
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
	.next			= &idle_sched_class,
	/* hooks */
	.enqueue_task 		= enqueue_task_mycfs,		/* done */
	.dequeue_task 		= dequeue_task_mycfs,		/* done */

	.check_preempt_curr 	= check_preempt_curr_mycfs,	/* ? */
	.pick_next_task 	= pick_next_task_mycfs,		/* done */
	.put_prev_task 		= put_prev_task_mycfs,
#ifdef CONFIG_SMP
	.select_task_rq 	= select_task_rq_mycfs,
#endif

	.set_curr_task 		= set_curr_task_mycfs,		/* done, for now */
	.task_tick 		= task_tick_mycfs,

	.get_rr_interval 	= get_rr_interval_mycfs,
	
	.prio_changed 		= prio_changed_mycfs,
	.switched_to 		= switched_to_mycfs,
	//.yeild_task		= yeild_task_mycfs,
};
