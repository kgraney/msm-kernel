#include <linux/rbtree.h>
#include <linux/sched.h>
#include "sched.h"

const struct sched_class mycfs_sched_class;

struct sched_mycfs_entity *__pick_first_mycfs_entity(struct mycfs_rq *mycfs_rq)
{
	struct rb_node *left = mycfs_rq->rb_leftmost;
	if (!left)
		return NULL;

	return rb_entry(left, struct sched_mycfs_entity, run_node);
}

void init_sched_mycfs_class(void)
{
	printk(KERN_DEBUG "MYCFS: initializing");
}

void init_mycfs_rq(struct mycfs_rq *mycfs_rq)
{
	printk(KERN_DEBUG "MYCFS: initializing rq=%p", mycfs_rq);
	mycfs_rq->tasks_timeline = RB_ROOT;
	mycfs_rq->min_vruntime = (u64)(-(1LL << 20));
}

static void __enqueue_entity(struct mycfs_rq *mycfs_rq, struct sched_mycfs_entity *ce, int flags)
{
	struct rb_node **link = &mycfs_rq->tasks_timeline.rb_node;
	struct rb_node *parent = NULL;
	struct sched_mycfs_entity *entry;
	int leftmost = 1;

	printk(KERN_DEBUG "MYCFS: enqueue entity (mycfs_rq=%p, tt=%p, link = %p)", mycfs_rq, &mycfs_rq->tasks_timeline, link);
	while (*link) {
		parent = *link;
		entry = rb_entry(parent, struct sched_mycfs_entity, run_node);

		if (ce->vruntime < entry->vruntime) {
			link = &parent->rb_left;
		} else {
			link = &parent->rb_right;
			leftmost = 0;
		}
	}

	if (leftmost)
		mycfs_rq->rb_leftmost = &ce->run_node;
	
	rb_link_node(&ce->run_node, parent, link);
	rb_insert_color(&ce->run_node, &mycfs_rq->tasks_timeline);
}

static void enqueue_task_mycfs(struct rq *rq, struct task_struct *p, int flags)
{
	struct mycfs_rq *mycfs_rq = &rq->mycfs;
	struct sched_mycfs_entity *ce = &p->ce;
	printk(KERN_DEBUG "MYCFS: task %d is runnable: %p", p->pid, ce);

	__enqueue_entity(mycfs_rq, ce, flags);
	mycfs_rq->nr_running++;
}

static void __dequeue_entity(struct mycfs_rq *mycfs_rq, struct sched_mycfs_entity *ce)
{
	if (mycfs_rq->rb_leftmost == &ce->run_node) {
		struct rb_node *next_node;

		next_node = rb_next(&ce->run_node);
		mycfs_rq->rb_leftmost = next_node;
	}

	rb_erase(&ce->run_node, &mycfs_rq->tasks_timeline);
}

static void dequeue_task_mycfs(struct rq *rq, struct task_struct *p, int flags)
{
	struct mycfs_rq *mycfs_rq = &rq->mycfs;
	struct sched_mycfs_entity *ce = &p->ce;
	printk(KERN_DEBUG "MYCFS: task %d is unrunnable: %p", p->pid, ce);

	__dequeue_entity(mycfs_rq, ce);
	mycfs_rq->nr_running--;
}

static void yield_task_mycfs(struct rq *rq)
{
}

static bool
yield_to_task_mycfs(struct rq *rq, struct task_struct *p, bool preempt)
{
	return true;
}

static void
check_preempt_wakeup(struct rq *rq, struct task_struct *p, int wake_flags)
{
}

static struct task_struct *pick_next_task_mycfs(struct rq *rq)
{
	struct task_struct *p;
	struct sched_mycfs_entity *ce;
	struct mycfs_rq *mycfs_rq = &rq->mycfs;

	if (!mycfs_rq->nr_running)
		return NULL;

	ce = __pick_first_mycfs_entity(mycfs_rq);
	p = container_of(ce, struct task_struct, ce);
	printk(KERN_DEBUG "MYCFS: returning pick_next_task: %p", p);
	return p;
}

static void put_prev_task_mycfs(struct rq *rq, struct task_struct *prev)
{
}

static int
select_task_rq_mycfs(struct task_struct *p, int sd_flag, int wake_flags)
{
	return smp_processor_id(); /* never change the processor of a process */
}

static void rq_online_mycfs(struct rq *rq)
{
	/* update_sysctl(); */
}

static void rq_offline_mycfs(struct rq *rq)
{
	/* update_sysctl(); */
}

static void task_waking_mycfs(struct task_struct *p)
{
}

static void set_curr_task_mycfs(struct rq *rq)
{
}

static void task_tick_mycfs(struct rq *rq, struct task_struct *curr, int queued)
{
}

static void task_fork_mycfs(struct task_struct *p)
{
}

static void
prio_changed_mycfs(struct rq *rq, struct task_struct *p, int oldprio)
{
	/* Do nothing; we don't care about priorities! */
}

static void switched_from_mycfs(struct rq *rq, struct task_struct *p)
{
}

static void switched_to_mycfs(struct rq *rq, struct task_struct *p)
{
}

static unsigned int get_rr_interval_mycfs(struct rq *rq, struct task_struct *task)
{
	return 0;
}

const struct sched_class mycfs_sched_class = {
	.next                   = &idle_sched_class,
	.enqueue_task           = enqueue_task_mycfs,
	.dequeue_task           = dequeue_task_mycfs,

	.yield_task             = yield_task_mycfs,
	.yield_to_task          = yield_to_task_mycfs,

	.check_preempt_curr     = check_preempt_wakeup,

	.pick_next_task         = pick_next_task_mycfs,
	.put_prev_task          = put_prev_task_mycfs,

#ifdef CONFIG_SMP
	.select_task_rq         = select_task_rq_mycfs,

	.rq_online              = rq_online_mycfs,
	.rq_offline             = rq_offline_mycfs,

	.task_waking            = task_waking_mycfs,
#endif

	.set_curr_task          = set_curr_task_mycfs,
	.task_tick		= task_tick_mycfs,
	.task_fork		= task_fork_mycfs,

	.prio_changed		= prio_changed_mycfs,
	.switched_from		= switched_from_mycfs,
	.switched_to		= switched_to_mycfs,

	.get_rr_interval	= get_rr_interval_mycfs,
};
