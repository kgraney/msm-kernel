/*
 * My Completely Fair Scheduling (SCHED_MYCFS) Class ()
 */

#include <linux/latencytop.h>
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

/*
 * scheduler tick hitting a task of our scheduling class:
 */
/*
static void task_tick_mycfs(struct rq *rq, struct task_struct *curr, int queued)
{
	struct mycfs_rq *mycfs_rq;
	struct mycfs_sched_entity *mycfs_se = &curr->mycfs_se;

	for_each_sched_entity(mycfs_se) {
		mycfs_rq = mycfs_rq_of(se);
		entity_tick(mycfs_rq, mycfs_se, queued);
	}
}
*/


const struct sched_class mycfs_sched_class = {
	.next			= &idle_sched_class,

	.dequeue_task		= dequeue_task_mycfs,
	.enqueue_task		= enqueue_task_mycfs,
	.check_preempt_curr	= check_preempt_curr_mycfs,

	.pick_next_task		= pick_next_task_mycfs,
	.put_prev_task		= put_prev_task_mycfs,

	.set_curr_task          = set_curr_task_mycfs,
	.task_tick		= task_tick_mycfs,

	.get_rr_interval	= get_rr_interval_mycfs,

	.prio_changed		= prio_changed_mycfs,
	.switched_to		= switched_to_mycfs,
};

