#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched/task.h>
#include <linux/sched/signal.h>
#include <asm/host_ops.h>
#include <asm/cpu.h>
#include <asm/sched.h>

extern _Atomic(bool) lkl_shutdown;

static int init_ti(struct thread_info *ti)
{
	LKL_TRACE("enter\n");

	ti->sched_sem = lkl_ops->sem_alloc(0);
	if (!ti->sched_sem)
		return -ENOMEM;

	ti->dead = false;
	ti->prev_sched = NULL;
	ti->tid = 0;
	ti->cloned_child = NULL;

	return 0;
}

unsigned long *alloc_thread_stack_node(struct task_struct *task, int node)
{
	struct thread_info *ti;

	LKL_TRACE("enter (task=%s node=%i)\n", task->comm, node);

	ti = kmalloc(sizeof(*ti), GFP_KERNEL);
	if (!ti)
		return NULL;

	if (init_ti(ti)) {
		kfree(ti);
		return NULL;
	}
	ti->task = task;


	return (unsigned long *)ti;
}

/*
 * The only new tasks created are kernel threads that have a predefined starting
 * point thus no stack copy is required.
 */
void setup_thread_stack(struct task_struct *p, struct task_struct *org)
{
	struct thread_info *ti = task_thread_info(p);
	struct thread_info *org_ti = task_thread_info(org);

	LKL_TRACE("enter\n");

	ti->flags = org_ti->flags;
	ti->preempt_count = org_ti->preempt_count;
	ti->addr_limit = org_ti->addr_limit;
}

static void kill_thread(struct thread_info *ti)
{
	struct task_struct *task = ti->task;

	LKL_TRACE("enter (task=%s task->state=%i task->flags=%i"
		  "ti=%p ti->flags=%i ti->TIF_NO_TERMINATION=%i )\n",
		  task->comm, task->state, task->flags, ti, ti->flags,
		  test_ti_thread_flag(ti, TIF_NO_TERMINATION));

	/* Check if we are killing an applicaton thread */
	if (!test_ti_thread_flag(ti, TIF_HOST_THREAD)) {
		ti->dead = true;
		lkl_ops->sem_up(ti->sched_sem);
		lkl_ops->thread_join(ti->tid);
	} else {
		/*
		 * If this is a task backing a host thread created by clone, then we
		 * need to destroy the associated host thread, but not exit LKL.
		 */
		if (test_ti_thread_flag(ti, TIF_CLONED_HOST_THREAD)) {
			clear_ti_thread_flag(ti, TIF_CLONED_HOST_THREAD);
			ti->dead = true;
			BUG_ON(!lkl_ops->thread_destroy_host);
			lkl_ops->thread_destroy_host(ti->tid, task_key);
			ti->tid = 0;
		/*
		 * Check if the host thread was killed due to its deallocation when
		 * the associated application thread terminated gracefully. If not,
		 * the thread has terminated due to a SYS_exit or a signal. In this
		 * case, we need to notify the host to initiate an LKL shutdown.
		 */
		} else if (!test_ti_thread_flag(ti, TIF_NO_TERMINATION)) {
			int exit_code = task->exit_code;
			int exit_status = exit_code >> 8;
			int received_signal = exit_code & 255;
			int exit_signal = task->exit_signal;

			LKL_TRACE(
				"terminating LKL (exit_state=%i exit_code=%i exit_signal=%i exit_status=%i "
				"received_signal=%i ti->dead=%i task->pid=%i "
				"task->tgid=%i ti->TIF_SCHED_JB=%i ti->TIF_SIGPENDING=%i)\n",
				task->exit_state, exit_code, exit_signal,
				exit_status, received_signal, ti->dead,
				task->pid, task->tgid,
				test_ti_thread_flag(ti, TIF_SCHED_JB),
				test_ti_thread_flag(ti, TIF_SIGPENDING));

			lkl_shutdown = true;

			/* Notify the LKL host to shut down */
			lkl_ops->terminate(exit_status, received_signal);
		}

		ti->dead = true;
	}
	lkl_ops->sem_free(ti->sched_sem);
}

void free_thread_stack(struct task_struct *tsk)
{
	struct thread_info *ti = task_thread_info(tsk);

	LKL_TRACE(
		"enter (task=%s task->TIF_HOST_THREAD=%i task->TIF_SIGPENDING=%i ti=%p current=%s)\n",
		tsk->comm, test_tsk_thread_flag(tsk, TIF_HOST_THREAD),
		test_tsk_thread_flag(tsk, TIF_SIGPENDING), ti, current->comm);

	kill_thread(ti);
	kfree(ti);
}

struct thread_info *_current_thread_info = &init_thread_union.thread_info;

/*
 * schedule() expects the return of this function to be the task that we
 * switched away from. Returning prev is not going to work because we are
 * actually going to return the previous taks that was scheduled before the
 * task we are going to wake up, and not the current task, e.g.:
 *
 * swapper -> init: saved prev on swapper stack is swapper
 * init -> ksoftirqd0: saved prev on init stack is init
 * ksoftirqd0 -> swapper: returned prev is swapper
 */
static struct task_struct *abs_prev = &init_task;

struct task_struct *__switch_to(struct task_struct *prev,
				struct task_struct *next)
{
	struct thread_info *_prev = task_thread_info(prev);
	struct thread_info *_next = task_thread_info(next);
	unsigned long _prev_flags = _prev->flags;
	struct lkl_jmp_buf _prev_jb;

	LKL_TRACE("%s=>%s\n", prev->comm, next->comm);

	_current_thread_info = task_thread_info(next);
	_next->prev_sched = prev;
	abs_prev = prev;

	BUG_ON(!_next->tid);
	lkl_cpu_change_owner(_next->tid);

	if (test_bit(TIF_SCHED_JB, &_prev_flags)) {
		/* Atomic. Must be done before wakeup next */
		clear_ti_thread_flag(_prev, TIF_SCHED_JB);
		_prev_jb = _prev->sched_jb;
	}
	LKL_TRACE("wake up sem=%p\n", _next->sched_sem);
	lkl_ops->sem_up(_next->sched_sem);
	if (test_bit(TIF_SCHED_JB, &_prev_flags)) {
		LKL_TRACE("longjmp back to task\n");
		lkl_ops->jmp_buf_longjmp(&_prev_jb, 1);
	} else {
		LKL_TRACE("down sem=%p\n", _prev->sched_sem);
		lkl_ops->sem_down(_prev->sched_sem);
	}

	if (_prev->dead)
		lkl_ops->thread_exit();

	return abs_prev;
}

int host_task_stub(void *unused)
{
	LKL_TRACE("enter\n");
	return 0;
}

void switch_to_host_task(struct task_struct *task)
{
	LKL_TRACE(
		"enter (task=%s current=%s task->TIF_HOST_THREAD=%i task->TIF_SIGPENDING=%i)\n",
		task->comm, current->comm,
		test_tsk_thread_flag(task, TIF_HOST_THREAD),
		test_tsk_thread_flag(task, TIF_SIGPENDING));

	if (WARN_ON(!test_tsk_thread_flag(task, TIF_HOST_THREAD)))
		return;

	task_thread_info(task)->tid = lkl_ops->thread_self();

	if (current == task)
		return;

	wake_up_process(task);
	thread_sched_jb();

	LKL_TRACE(
		"calling sem_down (task=%s task->TIF_HOST_THREAD=%i task->TIF_SIGPENDING=%i)\n",
		task->comm, test_tsk_thread_flag(task, TIF_HOST_THREAD),
		test_tsk_thread_flag(task, TIF_SIGPENDING));
	lkl_ops->sem_down(task_thread_info(task)->sched_sem);

	LKL_TRACE(
		"calling schedule_tail (task=%s task->TIF_HOST_THREAD=%i task->TIF_SIGPENDING=%i abs_prev=%s)\n",
		task->comm, test_tsk_thread_flag(task, TIF_HOST_THREAD),
		test_tsk_thread_flag(task, TIF_SIGPENDING), abs_prev->comm);
	schedule_tail(abs_prev);

	LKL_TRACE(
		"done (task=%s current=%s task->TIF_HOST_THREAD=%i task->TIF_SIGPENDING=%i)\n",
		task->comm, current->comm,
		test_tsk_thread_flag(task, TIF_HOST_THREAD),
		test_tsk_thread_flag(task, TIF_SIGPENDING));
}

struct thread_bootstrap_arg {
	struct thread_info *ti;
	int (*f)(void *);
	void *arg;
};

static void thread_bootstrap(void *_tba)
{
	LKL_TRACE("enter\n");

	struct thread_bootstrap_arg *tba = (struct thread_bootstrap_arg *)_tba;
	struct thread_info *ti = tba->ti;
	int (*f)(void *) = tba->f;
	void *arg = tba->arg;

	lkl_ops->sem_down(ti->sched_sem);
	kfree(tba);
	if (ti->prev_sched)
		schedule_tail(ti->prev_sched);

	f(arg);
	do_exit(0);
}

int copy_thread_tls(unsigned long clone_flags, unsigned long esp,
		unsigned long unused, struct task_struct *p, unsigned long tls)
{
	LKL_TRACE("enter\n");

	struct thread_info *ti = task_thread_info(p);
	struct thread_bootstrap_arg *tba;

	if ((int (*)(void *))esp == host_task_stub) {
		set_ti_thread_flag(ti, TIF_HOST_THREAD);
		return 0;
	}

	/*
	 * If we are creating a new userspace thread and are in the middle of a
	 * system call, create a new host thread coupled with this task.  The
	 * second check is necessary because we also hit this path when lazily
	 * binding a host thread to a new task on system call entry.
	 */
	void *pc = task_thread_info(current)->syscall_ret;
	if (pc && !(p->flags & PF_KTHREAD)) {
		/*
		 * If we have host support for creating new threads with fine-grained
		 * control over their initial state, use it to create a new host
		 * thread.
		 */
		if (lkl_ops->thread_create_host) {
			static unsigned long long clone_count = 0;
			set_ti_thread_flag(ti, TIF_HOST_THREAD);
			set_ti_thread_flag(ti, TIF_CLONED_HOST_THREAD);
			ti->tid = lkl_ops->thread_create_host(pc, (void*)esp, (void*)tls, task_key, p);
			snprintf(p->comm, sizeof(p->comm), "host_clone%llu", __sync_fetch_and_add(&clone_count, 1));
			current_thread_info()->cloned_child = p;
			return (ti->tid == 0) ? -ENOMEM : 0;
		}
		return -ENODEV;
	}

	tba = kmalloc(sizeof(*tba), GFP_KERNEL);
	if (!tba)
		return -ENOMEM;

	tba->f = (int (*)(void *))esp;
	tba->arg = (void *)unused;
	tba->ti = ti;

	ti->tid = lkl_ops->thread_create(thread_bootstrap, tba);
	if (!ti->tid) {
		kfree(tba);
		return -ENOMEM;
	}

	return 0;
}

void show_stack(struct task_struct *task, unsigned long *esp)
{
	LKL_TRACE("enter\n");
}

/**
 * This is called before the kernel initializes, so no kernel calls (including
 * printk) can't be made yet.
 */
void threads_init(void)
{
	int ret;
	struct thread_info *ti = &init_thread_union.thread_info;

	LKL_TRACE("enter\n");

	ret = init_ti(ti);
	if (ret < 0) {
		lkl_printf("lkl: failed to allocate thread_info struct\n");
	}

	ti->tid = lkl_ops->thread_self();
}

void threads_cleanup(void)
{
	struct task_struct *p, *t;

	LKL_TRACE("enter\n");

	for_each_process_thread(p, t) {
		struct thread_info *ti = task_thread_info(t);

		if (t->pid != 1 && !test_ti_thread_flag(ti, TIF_HOST_THREAD))
			WARN(!(t->flags & PF_KTHREAD),
			     "non kernel thread task %s\n", t->comm);
		WARN(t->state == TASK_RUNNING,
		     "thread %s still running while halting\n", t->comm);

		kill_thread(ti);
	}

	lkl_ops->sem_free(init_thread_union.thread_info.sched_sem);
}
