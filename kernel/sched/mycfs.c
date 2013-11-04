/*
 * My Completely Fair Scheduling (SCHED_MYCFS) Class ()
 */

#include <linux/latencytop.h>
#include <linux/rbtree.h>
#include <linux/sched.h>
#include <linux/cpumask.h>
#include <linux/slab.h>
#include <linux/profile.h>
#include <linux/interrupt.h>

#include <trace/events/sched.h>

#include "sched.h"

/*
 * Preemption latency for CPU-bound tasks:  10ms
 */

unsigned int mycfs_sched_latency = 10000000ULL;

const struct sched_class mycfs_sched_class;

static inline int mycfs_entity_before(struct sched_mycfs_entity *a,
				struct sched_mycfs_entity *b)
{
	return (s64)(a->vruntime - b->vruntime) < 0;
}

static inline struct mycfs_rq *task_mycfs_rq(struct task_struct *p)
{
	return &task_rq(p)->mycfs;
}

static inline struct task_struct *task_of(struct sched_mycfs_entity *mycfs_se)
{
	return container_of(mycfs_se, struct task_struct, mycfs_se);
}

static inline struct mycfs_rq *mycfs_rq_of(struct sched_mycfs_entity *mycfs_se)
{
	struct task_struct *p = task_of(mycfs_se);
	struct rq *rq = task_rq(p);

	return &rq->mycfs;
}

void init_sched_mycfs_class(void)
{

}

void init_mycfs_rq(struct mycfs_rq *mycfs_rq)
{
	mycfs_rq->tasks_timeline = RB_ROOT;
	mycfs_rq->min_vruntime = (u64)(-(1LL << 20));
}

static void __dequeue_mycfs_entity(struct mycfs_rq *mycfs_rq, struct sched_mycfs_entity *mycfs_se)
{
	if (mycfs_rq->rb_leftmost == &mycfs_se->run_node)
	{
		struct rb_node *next_node;

		next_node = rb_next(&mycfs_se->run_node);
		mycfs_rq->rb_leftmost = next_node;
	}

	rb_erase(&mycfs_se->run_node, &mycfs_rq->tasks_timeline);
}

static void dequeue_task_mycfs(struct rq *rq, struct task_struct *p, int flags)
{
	struct mycfs_rq *mycfs_rq;
	struct sched_mycfs_entity *mycfs_se = &p->mycfs_se;

	mycfs_rq = &rq->mycfs;
	__dequeue_mycfs_entity(mycfs_rq, mycfs_se);
	mycfs_rq->nr_running--;
}

static void __enqueue_mycfs_entity(struct mycfs_rq *mycfs_rq, struct sched_mycfs_entity *mycfs_se)
{
	struct rb_node **link = &mycfs_rq->tasks_timeline.rb_node;
	struct rb_node *parent = NULL;
	struct sched_mycfs_entity *entry;
	int leftmost = 1;

	while (*link) {
		parent = *link;
		entry = rb_entry(parent, struct sched_mycfs_entity, run_node);
		if (mycfs_entity_before(mycfs_se, entry)) {
			link = &parent->rb_left;
		} else {
			link = &parent->rb_right;
			leftmost = 0;
		}
	}
	if (leftmost)
		mycfs_rq->rb_leftmost = &mycfs_se->run_node;
	rb_link_node(&mycfs_se->run_node, parent, link);
	rb_insert_color(&mycfs_se->run_node, &mycfs_rq->tasks_timeline);
}

static void enqueue_task_mycfs(struct rq *rq, struct task_struct *p, int flags)
{
	struct mycfs_rq *mycfs_rq;
	struct sched_mycfs_entity *mycfs_se = &p->mycfs_se;

	mycfs_rq = &rq->mycfs;
	if(!(mycfs_se->on_rq))
	{
		__enqueue_mycfs_entity(mycfs_rq, mycfs_se);
		mycfs_rq->nr_running++;
	}
}

static void __clear_mycfs_buddies_last(struct sched_mycfs_entity *mycfs_se)
{
	struct mycfs_rq *mycfs_rq = mycfs_rq_of(mycfs_se);
	if (mycfs_rq->last == mycfs_se)
		mycfs_rq->last = NULL;
	else
		break;
}

static void __clear_mycfs_buddies_next(struct sched_mycfs_entity *mycfs_se)
{
	struct mycfs_rq *mycfs_rq = mycfs_rq_of(mycfs_se);
	if (mycfs_rq->next == mycfs_se)
		mycfs_rq->next = NULL;
	else
		break;
}

static void __clear_mycfs_buddies_skip(struct sched_mycfs_entity *mycfs_se)
{
	struct mycfs_rq *mycfs_rq = mycfs_rq_of(mycfs_se);
	if (mycfs_rq->skip == mycfs_se)
		mycfs_rq->skip = NULL;
	else
		break;
}

static void clear_mycfs_buddies(struct mycfs_rq *mycfs_rq, struct sched_mycfs_entity *mycfs_se)
{
	if (mycfs_rq->last == mycfs_se)
		__clear_mycfs_buddies_last(mycfs_se);

	if (mycfs_rq->next == mycfs_se)
		__clear_mycfs_buddies_next(mycfs_se);

	if (mycfs_rq->skip == mycfs_se)
		__clear_mycfs_buddies_skip(mycfs_se);
}

static void set_skip_mycfs_buddy(struct sched_mycfs_entity *mycfs_se)
{
	mycfs_rq_of(mycfs_se)->skip = mycfs_se;
}

static void update_min_vruntime_mycfs(struct mycfs_rq *mycfs_rq)
{
	u64 vruntime = mycfs_rq->min_vruntime;

	if (mycfs_rq->curr)
		vruntime = mycfs_rq->curr->vruntime;

	if (mycfs_rq->rb_leftmost) {
		struct sched_mycfs_entity *mycfs_se = rb_entry(mycfs_rq->rb_leftmost,
						   struct sched_mycfs_entity,
						   run_node);
		if (!mycfs_rq->curr)
			vruntime = mycfs_se->vruntime;
		else
			vruntime = min_vruntime(vruntime, mycfs_se->vruntime);
	}
	mycfs_rq->min_vruntime = max_vruntime(mycfs_rq->min_vruntime, vruntime);
}

static inline void __update_mycfs_curr(struct mycfs_rq *mycfs_rq, struct sched_entity *curr,
	      unsigned long delta_exec)
{
	unsigned long delta_exec_weighted;

	schedstat_set(curr->statistics.exec_max,
		      max((u64)delta_exec, curr->statistics.exec_max));

	curr->sum_exec_runtime += delta_exec;
	schedstat_add(cfs_rq, exec_clock, delta_exec);
	delta_exec_weighted = calc_delta_fair(delta_exec, curr);

	curr->vruntime += delta_exec_weighted;
	update_min_vruntime_mycfs(mycfs_rq);
}

static void update_mycfs_curr(struct mycfs_rq *mycfs_rq)
{
	struct sched_entity *curr = mycfs_rq->curr;
	u64 now = rq_of(cfs_rq)->clock_task;
	unsigned long delta_exec;

	if (unlikely(!curr))
		return;
	delta_exec = (unsigned long)(now - curr->exec_start);
	if (!delta_exec)
		return;

	__update_mycfs_curr(mycfs_rq, curr, delta_exec);
	curr->exec_start = now;
	if (entity_is_task(curr)) {
		struct task_struct *curtask = task_of(curr);

		trace_sched_stat_runtime(curtask, delta_exec, curr->vruntime);
		cpuacct_charge(curtask, delta_exec);
		account_group_exec_runtime(curtask, delta_exec);
	}
	account_cfs_rq_runtime(cfs_rq, delta_exec);
}


static void yield_task_mycfs(struct rq *rq)
{
	struct task_struct *curr = rq->curr;
	struct mycfs_rq *mycfs_rq = task_mycfs_rq(curr);
	struct sched_mycfs_entity *mycfs_se = &curr->mycfs_se;

	if (unlikely(rq->nr_running == 1))
		return;
	clear_mycfs_buddies(mycfs_rq, mycfs_se);
	if (curr->policy != SCHED_BATCH) {
		update_rq_clock(rq);
		update_mycfs_curr(mycfs_rq);
		rq->skip_clock_update = 1;
	}
	set_skip_buddy(mycfs_se);
}

static bool yield_to_task_mycfs(struct rq *rq, struct task_struct *p, bool preempt)
{
	struct sched_mycfs_entity *mycfs_se = &p->mycfs_se;

	if (!mycfs_se->on_rq)
		return false;
	yield_task_mycfs(rq);
	return true;
}

static void check_preempt_wakeup(struct rq *rq, struct task_struct *p, int wake_flags)
{

}

struct sched_mycfs_entity *__pick_first_mycfs_entity(struct mycfs_rq *mycfs_rq)
{
	struct rb_node *left = mycfs_rq->rb_leftmost;

	if (!left)
		return NULL;
	return rb_entry(left, struct sched_mycfs_entity, run_node);
}

static struct task_struct *pick_next_task_mycfs(struct rq *rq)
{
	struct task_struct *ts_ptr;
	struct mycfs_rq *mycfs_rq;
	struct sched_mycfs_entity *mycfs_se;

	mycfs_rq = &rq->mycfs;
	if(mycfs_rq->nr_running == 0)
		return NULL;
	mycfs_se = __pick_first_mycfs_entity(mycfs_rq);
	ts_ptr = container_of(mycfs_se, struct task_struct, mycfs_se);
	return ts_ptr;
}

static void put_prev_task_mycfs(struct rq *rq, struct task_struct *prev)
{

}

static void set_curr_task_mycfs(struct rq *rq)
{

}

static void task_tick_mycfs(struct rq *rq, struct task_struct *curr, int queued)
{
//	struct mycfs_rq *mycfs_rq;
//	struct mycfs_sched_entity *mycfs_se = &curr->mycfs_se;
//
//	for_each_sched_entity(mycfs_se)
//	{
//		mycfs_rq = mycfs_rq_of(se);
//		entity_tick(mycfs_rq, mycfs_se, queued);
//	}
}

static unsigned int get_rr_interval_mycfs(struct rq *rq, struct task_struct *task)
{
	return 0;
}

static void prio_changed_mycfs(struct rq *rq, struct task_struct *p, int oldprio)
{

}

static void switched_to_mycfs(struct rq *rq, struct task_struct *p)
{

}

const struct sched_class mycfs_sched_class = {
	.next			= &idle_sched_class,

	.dequeue_task		= dequeue_task_mycfs,
	.enqueue_task		= enqueue_task_mycfs,
	.check_preempt_curr	= check_preempt_wakeup,

	.yield_task		= yield_task_mycfs,
	.yield_to_task		= yield_to_task_mycfs,

	.pick_next_task		= pick_next_task_mycfs,
	.put_prev_task		= put_prev_task_mycfs,

	.set_curr_task          = set_curr_task_mycfs,
	.task_tick		= task_tick_mycfs,

	.get_rr_interval	= get_rr_interval_mycfs,

	.prio_changed		= prio_changed_mycfs,
	.switched_to		= switched_to_mycfs,
};

