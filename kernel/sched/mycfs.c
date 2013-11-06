#include <linux/rbtree.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include "sched.h"

unsigned int sysctl_sched_latency = 10000000ULL; /* 10 ms schedule latency */
const struct sched_class mycfs_sched_class;

static void
set_next_entity(struct mycfs_rq *mycfs_rq, struct sched_mycfs_entity *ce);

static inline u64 max_vruntime(u64 min_vruntime, u64 vruntime)
{
	s64 delta = (s64)(vruntime - min_vruntime);
	if (delta > 0)
		min_vruntime = vruntime;

	return min_vruntime;
}

static inline u64 min_vruntime(u64 min_vruntime, u64 vruntime)
{
	s64 delta = (s64)(vruntime - min_vruntime);
	if (delta < 0)
		min_vruntime = vruntime;

	return min_vruntime;
}

static void update_min_vruntime(struct mycfs_rq *mycfs_rq)
{
	u64 vruntime = mycfs_rq->min_vruntime;

	if (mycfs_rq->curr)
		vruntime = mycfs_rq->curr->vruntime;

	if (mycfs_rq->rb_leftmost) {
		struct sched_mycfs_entity *ce = rb_entry(mycfs_rq->rb_leftmost,
						   struct sched_mycfs_entity,
						   run_node);

		if (!mycfs_rq->curr)
			vruntime = ce->vruntime;
		else
			vruntime = min_vruntime(vruntime, ce->vruntime);
	}

	mycfs_rq->min_vruntime = max_vruntime(mycfs_rq->min_vruntime, vruntime);
}

static void update_curr(struct mycfs_rq *mycfs_rq)
{
	struct rq *rq = mycfs_rq->rq;
	struct sched_mycfs_entity *curr = mycfs_rq->curr;
	u64 now = rq->clock_task;
	unsigned long delta_exec;
	if (unlikely(!curr))
		return;

	delta_exec = (unsigned long)(now - curr->exec_start);

	curr->vruntime += delta_exec; /* TODO: Fix! */
	update_min_vruntime(mycfs_rq);

	/*printk(KERN_DEBUG "MYCFS: %d vruntime=%llu min=%llu", container_of(curr, struct task_struct, ce)->pid, curr->vruntime, mycfs_rq->min_vruntime);*/

	curr->exec_start = now;
}



struct sched_mycfs_entity *__pick_first_mycfs_entity(struct mycfs_rq *mycfs_rq)
{
	struct rb_node *left = mycfs_rq->rb_leftmost;
	if (!left)
		return NULL;

	return rb_entry(left, struct sched_mycfs_entity, run_node);
}

void init_mycfs_rq(struct mycfs_rq *mycfs_rq)
{
	printk(KERN_DEBUG "MYCFS: initializing rq=%p", mycfs_rq);
	mycfs_rq->tasks_timeline = RB_ROOT;
	mycfs_rq->min_vruntime = (u64)(-(1LL << 20));
}

static void __enqueue_entity(struct mycfs_rq *mycfs_rq, struct sched_mycfs_entity *ce)
{
	struct rb_node **link = &mycfs_rq->tasks_timeline.rb_node;
	struct rb_node *parent = NULL;
	struct sched_mycfs_entity *entry;
	int leftmost = 1;

	/* printk(KERN_DEBUG "MYCFS: enqueue entity (mycfs_rq=%p, tt=%p, link = %p)", mycfs_rq, &mycfs_rq->tasks_timeline, link); */
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
	if (ce->on_rq)
		return;

	printk(KERN_DEBUG "MYCFS: task %d is runnable: %p", p->pid, ce);

	mycfs_rq->rq = rq;
	ce->mycfs_rq = mycfs_rq;

	ce->vruntime += mycfs_rq->min_vruntime;
	update_curr(mycfs_rq);

	__enqueue_entity(mycfs_rq, ce);
	ce->on_rq = 1;
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
	ce->on_rq = 0;
	mycfs_rq->nr_running--;
	update_min_vruntime(mycfs_rq);
}

static void yield_task_mycfs(struct rq *rq)
{
	struct mycfs_rq *mycfs_rq = &rq->mycfs;
	if (unlikely(rq->nr_running == 1))
		return;

	update_rq_clock(rq);
	update_curr(mycfs_rq);
	rq->skip_clock_update = 1;
}

static bool
yield_to_task_mycfs(struct rq *rq, struct task_struct *p, bool preempt)
{
	struct sched_mycfs_entity *ce = &p->ce;
	if (!ce->on_rq)
		return false;

	if (unlikely(rq->nr_running == 1))
		return true;

	update_curr(&rq->mycfs);
	return true;
}

static void
check_preempt_wakeup(struct rq *rq, struct task_struct *p, int wake_flags)
{
	struct task_struct *curr = rq->curr;
	struct sched_mycfs_entity *ce = &curr->ce, *pce = &p->ce;
	/* struct mycfs_rq *mycfs_rq = &rq->mycfs; */

	if (unlikely(ce == pce))
		return;

	printk(KERN_DEBUG "MYCFS: preempt");
	/*__enqueue_entity(mycfs_rq, ce); */
	resched_task(curr);
	/* set_next_entity(mycfs_rq, &p->ce); */
}

static void
check_preempt_tick(struct mycfs_rq *mycfs_rq, struct sched_mycfs_entity *curr)
{
	struct sched_mycfs_entity *ce;
	s64 delta;

	printk(KERN_DEBUG "MYCFS: fo %d %d", curr->limit, curr->run_ticks);
	/* reschedule if portion of 100 ms slice exceeded */
	if (curr->limit && curr->run_ticks >= curr->limit) {
		printk(KERN_DEBUG "MYCFS: tick exceeded %d", curr->run_ticks);
		resched_task(mycfs_rq->rq->curr);
	}

	ce = __pick_first_mycfs_entity(mycfs_rq);
	delta = curr->vruntime - ce->vruntime;
	if (delta < 0)
		return;

	if (delta > (unsigned long)1e8) /* fixed 100ms slice */
		resched_task(mycfs_rq->rq->curr);
}

static void
set_next_entity(struct mycfs_rq *mycfs_rq, struct sched_mycfs_entity *ce)
{
	/*if (ce->on_rq)
		__dequeue_entity(mycfs_rq, ce);*/
	mycfs_rq->curr = ce;
}

static struct task_struct *pick_next_task_mycfs(struct rq *rq)
{
	struct task_struct *p;
	struct sched_mycfs_entity *ce;
	struct mycfs_rq *mycfs_rq = &rq->mycfs;
	struct rb_node *node;
	int i = 0;

	if (!mycfs_rq->nr_running)
		return NULL;

	for (node = rb_first(&mycfs_rq->tasks_timeline); node; node = rb_next(node)) {
		ce = rb_entry(node, struct sched_mycfs_entity, run_node);
		p = container_of(ce, struct task_struct, ce);
		printk(KERN_DEBUG "MYCFS: rbtree(%d): pid=%d vr=%llu", i, p->pid, p->ce.vruntime);
		i++;
	}

	ce = __pick_first_mycfs_entity(mycfs_rq);
	set_next_entity(mycfs_rq, ce);
	p = container_of(ce, struct task_struct, ce);
	printk(KERN_DEBUG "MYCFS: picking task %d (vr=%llu)", p->pid, p->ce.vruntime);
	return p;
}

static void put_prev_task_mycfs(struct rq *rq, struct task_struct *prev)
{
	/*struct sched_mycfs_entity *ce = &prev->ce; */
	struct mycfs_rq *mycfs_rq = &rq->mycfs;
	if (prev->on_rq) {
		update_curr(mycfs_rq);
		/*__enqueue_entity(mycfs_rq, ce);*/
	}
	mycfs_rq->curr = NULL;
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
	struct sched_mycfs_entity *ce = &p->ce;
	struct mycfs_rq *mycfs_rq = ce->mycfs_rq;

	ce->vruntime -= mycfs_rq->min_vruntime;
}

static void set_curr_task_mycfs(struct rq *rq)
{
	struct mycfs_rq *mycfs_rq = &rq->mycfs;
	struct sched_mycfs_entity *ce = &rq->curr->ce;
	set_next_entity(mycfs_rq, ce);
}

static void
entity_tick(struct mycfs_rq *mycfs_rq, struct sched_mycfs_entity *curr, int queued)
{
	if (mycfs_rq->nr_running > 1)
		check_preempt_tick(mycfs_rq, curr);
}

/* Called at some Hz interval (period about 10 ms, empirically) */
static void task_tick_mycfs(struct rq *rq, struct task_struct *curr, int queued)
{
	struct mycfs_rq *mycfs_rq = &rq->mycfs;
	struct sched_mycfs_entity *ce = &curr->ce;
	u64 now = rq->clock_task;
	return;

	ce->run_ticks++;
	if (ce->run_ticks == 10) { /* edge of 100 ms period */
		printk(KERN_DEBUG "MYCFS: new period for %d =  %llu", curr->pid, now);
		ce->run_ticks += 10;
	}

	mycfs_rq->rq = rq;
	update_curr(mycfs_rq);

	entity_tick(mycfs_rq, ce, queued);
	/* struct sched_mycfs_entity *ce = &curr->ce;*/
}

static void task_fork_mycfs(struct task_struct *p)
{
	struct mycfs_rq *mycfs_rq = p->ce.mycfs_rq;
	struct sched_mycfs_entity *ce = &p->ce, *curr;
	curr = mycfs_rq->curr;
	update_curr(mycfs_rq);

	if (curr)
		ce->vruntime = curr->vruntime;
	resched_task(mycfs_rq->rq->curr);
	ce->vruntime -= mycfs_rq->min_vruntime;
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
	if(!p->ce.on_rq)
		return;

	resched_task(rq->curr);
}

static unsigned int get_rr_interval_mycfs(struct rq *rq, struct task_struct *task)
{
	return 0; /* This should be fine */
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


SYSCALL_DEFINE2(sched_setlimit, pid_t, pid, int, limit)
{
	struct task_struct* ts = find_task_by_vpid(pid);
	ts->ce.limit = limit;
	printk(KERN_DEBUG "MYCFS: setlimit for %d to %d", pid, limit);
	return 0;
}
