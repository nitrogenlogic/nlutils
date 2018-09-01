/*
 * Thread-related functions.
 * Copyright (C)2014, 2018 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <sys/prctl.h>

#include "nlutils.h"

// TODO: Use thread-local storage and/or thread info structure to set longer names without affecting process names

/*
 * Sets the name of the current thread or process, truncating to at most 15
 * bytes plus NUL.  Returns 0 on success, or a positive errno-style error code
 * on failure.
 */
int nl_set_threadname(char *name)
{
	char buf[16];
	int ret = 0;

	if(CHECK_NULL(name)) {
		return EFAULT;
	}

	snprintf(buf, 16, "%s", name);

#if defined(__GLIBC__) && __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 12
	ret = pthread_setname_np(pthread_self(), buf);
#else /* glibc version test */
	if(prctl(PR_SET_NAME, (unsigned long)buf, 0, 0, 0)) {
		ret = errno;
	}
#endif /* glibc version test */

	return ret;
}

/*
 * Stores the name of the current thread in the given buffer, which must be
 * able to hold at least 16 bytes, including NUL.  A 16-byte name will be
 * truncated to 15 bytes plus NUL.  Returns 0 on success, or a positive
 * errno-style error code on failure.
 */
int nl_get_threadname(char *name)
{
	int ret = 0;

	if(CHECK_NULL(name)) {
		return EFAULT;
	}

#if defined(__GLIBC__) && __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 12
	ret = pthread_getname_np(pthread_self(), name, 16);
#else /* glibc version test */
	if(prctl(PR_GET_NAME, (unsigned long)name, 0, 0, 0)) {
		ret = errno;
	}
	name[15] = 0;
#endif /* glibc version test */

	return ret;
}

/*
 * Internal thread execution function.  Sets the thread's name and performs any
 * other housekeeping, then calls the user's thread function.
 */
static void *nl_runthread_func(void *data)
{
	struct nl_thread *info = (struct nl_thread *)data;

	if(info->name[0]) {
		nl_set_threadname(info->name);
	} else {
		nl_get_threadname(info->name);
	}

	return info->func(info->data);
}

/*
 * Creates an empty thread context for use with nl_create_thread() and related
 * functions.  Returns NULL and sets errno on error (via calloc()).
 */
struct nl_thread_ctx *nl_create_thread_context()
{
	struct nl_thread_ctx *ctx;
	int ret;

	ctx = calloc(1, sizeof(struct nl_thread_ctx));
	if(ctx == NULL) {
		return NULL;
	}

	ctx->main_thread = pthread_self();

	ret = nl_create_mutex(&ctx->lock, -1);
	if(ret) {
		ERROR_OUT("Error creating thread context lock: %d (%s)\n", ret, strerror(ret));
		free(ctx);
		return NULL;
	}

	return ctx;
}

/*
 * Joins all threads on the given context, then frees the context's memory.
 */
void nl_destroy_thread_context(struct nl_thread_ctx *ctx)
{
	char name[16];
	int ret;

	if(CHECK_NULL(ctx)) {
		return;
	}

	ret = pthread_mutex_lock(&ctx->lock);
	if(ret) {
		ERROR_OUT("Warning: error locking thread context mutex: %d (%s)\n", ret, strerror(ret));
	}

	while(ctx->threads) {
		strncpy(name, ctx->threads->name, sizeof(name));

		ret = nl_join_thread(ctx->threads, NULL);
		if(ret) {
			ERROR_OUT("Error joining thread %s: %d (%s)\n", name, ret, strerror(ret));
		}
	}

	ret = pthread_mutex_unlock(&ctx->lock);
	if(ret) {
		ERROR_OUT("Warning: error unlocking thread context mutex: %d (%s)\n", ret, strerror(ret));
	}

	ret = pthread_mutex_destroy(&ctx->lock);
	if(ret) {
		ERROR_OUT("Error destroying thread context mutex: %d (%s)\n", ret, strerror(ret));
	}

	free(ctx);
}

/*
 * Calls pthread_create to create a thread, storing the pthread_t handle in a
 * new entry in the given thread context.  A pointer to the thread's info
 * structure is stored in **info if it is not NULL (note that the pthread_t
 * handle for the thread is returned in **info).  If *name is not NULL, then
 * the thread's name will be set to name immediately after creation.  Returns 0
 * on success, a positive errno-like value on error.
 */
int nl_create_thread(struct nl_thread_ctx *ctx, const pthread_attr_t *attr, void *(*func)(void *), void *data, char *name, struct nl_thread **info)
{
	struct nl_thread *thread;
	int ret;

	if(CHECK_NULL(ctx) || CHECK_NULL(func)) {
		return EINVAL;
	}

	ret = pthread_mutex_lock(&ctx->lock);
	if(ret) {
		ERROR_OUT("Error locking thread context mutex: %d (%s)\n", ret, strerror(ret));
		return ret;
	}

	thread = calloc(1, sizeof(struct nl_thread));
	if(thread == NULL) {
		ret = errno;
		ERRNO_OUT("Error allocating memory for thread thread");
		pthread_mutex_unlock(&ctx->lock);
		return ret;
	}

	thread->ctx = ctx;

	thread->func = func;
	thread->data = data;
	if(name != NULL) {
		snprintf(thread->name, sizeof(thread->name), "%s", name);
	}

	ret = pthread_create(&thread->thread, attr, nl_runthread_func, thread);
	if(ret) {
		ERROR_OUT("Error starting thread: %d (%s)\n", ret, strerror(ret));
		free(thread);
		pthread_mutex_unlock(&ctx->lock);
		return ret;
	}

	thread->next = ctx->threads;
	ctx->threads = thread;

	if(info != NULL) {
		*info = thread;
	}

	ret = pthread_mutex_unlock(&ctx->lock);
	if(ret) {
		ERROR_OUT("Error unlocking thread context mutex: %d (%s)\n", ret, strerror(ret));
		pthread_cancel(thread->thread);
		ctx->threads = thread->next;
		free(thread);
		return ret;
	}

	return 0;
}

/*
 * Attempts to join the given thread, then removes it from its thread context.
 * The thread info will be removed from the context and its memory freed
 * regardless of whether joining succeeds.  The return value of the thread's
 * main function will be stored in **result if it is not NULL (see the manual
 * page for pthread_join()).  Returns 0 on success, an errno-like value on
 * error.
 */
int nl_join_thread(struct nl_thread *info, void **result)
{
	struct nl_thread *prev, *cur;
	int ret;

	if(CHECK_NULL(info)) {
		return EINVAL;
	}

	ret = pthread_mutex_lock(&info->ctx->lock);
	if(ret) {
		ERROR_OUT("Warning: error locking thread context lock: %d (%s)\n", ret, strerror(ret));
	}

	// Find the thread in the context list (alternatively, could use a
	// ->prev member for O(1) removal)
	for(prev = NULL, cur = info->ctx->threads; cur; prev = cur, cur = cur->next) {
		if(cur == info) {
			if(prev == NULL) {
				info->ctx->threads = cur->next;
			} else {
				prev->next = cur->next;
			}

			break;
		}
	}

	ret = pthread_mutex_unlock(&info->ctx->lock);
	if(ret) {
		ERROR_OUT("Warning: error unlocking thread context lock: %d (%s)\n", ret, strerror(ret));
	}

	if(cur == NULL) {
		ERROR_OUT("Warning: joining a thread that wasn't found in its context.\n");
	}

	ret = pthread_join(info->thread, result);

	free(info);

	return ret;
}

/*
 * Sets the scheduling class (e.g. SCHED_OTHER, SCHED_FIFO) and priority of the
 * given thread, or the current thread if thread is NULL.  Note that this is
 * not the same thing as niceness.  Returns 0 on success, an errno-like value
 * on error.
 */
int nl_set_thread_priority(struct nl_thread *thread, int sched_class, int prio)
{
	pthread_t tid;
	int ret;

	if(thread == NULL) {
		tid = pthread_self();
	} else {
		tid = thread->thread;
	}

	ret = pthread_setschedparam(tid, sched_class, &(struct sched_param){.sched_priority = prio});
	if(ret) {
		ERROR_OUT("Error setting thread class and priority: %s (%d)\n", strerror(ret), ret);
		return ret;
	}

	return 0;
}

/*
 * Waits up to timeout_us microseconds to lock the given mutex, trying roughly
 * every millisecond.  Returns 0 on success, EBUSY if the timeout expired, or a
 * different error code if acquiring the lock failed.
 */
static int nl_timeout_lock(pthread_mutex_t *mutex, int timeout_us)
{
	// TODO: expose this function and add tests for it
	int slept = 0;
	int ret;

	if(timeout_us <= 0) {
		ret = pthread_mutex_lock(mutex);
	} else {
		do {
			ret = pthread_mutex_trylock(mutex);
			if(ret == EBUSY) {
				nl_usleep(1000);
				slept += 1000;
			}
		} while(slept < timeout_us && ret == EBUSY);
	}

	return ret;
}

/*
 * Calls the given callback for each thread that was created in the given
 * threading context (thus the main thread that created the context is
 * excluded).
 *
 * If lock_timeout_us is greater than zero, then iteration will proceed anyway
 * after approximately lock_timeout_us microseconds even if the thread list
 * lock cannot be obtained.  This could happen if another thread is already
 * running nl_destroy_thread_context(), and nl_iterate_threads() is called from
 * a signal handler, for example.
 *
 * The list of threads should not be modified by the callback (no creation or
 * joining of threads).
 */
void nl_iterate_threads(struct nl_thread_ctx *ctx, int lock_timeout_us, nl_thread_iterator cb, void *cb_data)
{
	int ret;

	if(CHECK_NULL(ctx) || CHECK_NULL(cb)) {
		return;
	}

	ret = nl_timeout_lock(&ctx->lock, lock_timeout_us);
	if(ret) {
		if(ret == EBUSY && lock_timeout_us > 0) {
			ERROR_OUT("Warning: ignoring lock timeout when iterating threads\n");
		} else {
			ERROR_OUT("Error locking thread context lock for iteration: %d (%s)\n", ret, strerror(ret));
			return;
		}
	}

	struct nl_thread *t = ctx->threads;
	while(t) {
		cb(t, cb_data);
		t = t->next;
	}

	ret = pthread_mutex_unlock(&ctx->lock);
	if(ret) {
		ERROR_OUT("Warning: error unlocking thread context lock after iteration: %d (%s)\n", ret, strerror(ret));
	}
}

/*
 * Callback for nl_signal_threads().  Signals one thread, then waits 25ms if
 * there are more threads.
 */
static void nl_signal_one_thread(struct nl_thread *t, void *data)
{
	pthread_t self = pthread_self();
	int signum = *(int *)data;

	if(!pthread_equal(t->thread, self)) {
		int ret = pthread_kill(t->thread, signum);
		if(ret) {
			ERROR_OUT("Error sending signal %d (%s) to thread %s: %d (%s)\n",
					signum, strsignal(signum), t->name, ret, strerror(ret));
		}

		if(t->next) {
			nl_usleep(25000);
		}
	}
}

/*
 * Sends the given signal to all the threads in the given context, excluding
 * the main thread that created the context and the current thread (if it's one
 * of the threads in the context).  Waits 25ms between threads.
 */
void nl_signal_threads(struct nl_thread_ctx *ctx, int signum)
{
	nl_iterate_threads(ctx, 250000, nl_signal_one_thread, &signum);
}

/*
 * Creates a priority inheritance mutex (PTHREAD_PRIO_INHERIT) in *mutex.  Pass
 * PTHREAD_MUTEX_NORMAL for type to create a standard mutex.  For a recursive
 * mutex, pass -1 or PTHREAD_MUTEX_RECURSIVE.  For an error checking mutex,
 * pass PTHREAD_MUTEX_ERRORCHECK.  Returns 0 on success, or an ERRNO-like value
 * on error.
 */
int nl_create_mutex(pthread_mutex_t *mutex, int type) // TODO: Add robustness parameter
{
	pthread_mutexattr_t mtxattr;
	int ret;

	if(type == -1) {
		type = PTHREAD_MUTEX_RECURSIVE;
	}

	ret = pthread_mutexattr_init(&mtxattr);
	if(ret) {
		ERROR_OUT("Error initializing mutex attributes: %s\n", strerror(ret));
		return ret;
	}

	ret = pthread_mutexattr_settype(&mtxattr, type);
	if(ret) {
		ERROR_OUT("Error setting mutex as recursive: %s\n", strerror(ret));
		pthread_mutexattr_destroy(&mtxattr);
		return ret;
	}

#ifdef _POSIX_THREAD_PRIO_INHERIT
	ret = pthread_mutexattr_setprotocol(&mtxattr, PTHREAD_PRIO_INHERIT);
	if(ret) {
		ERROR_OUT("Error setting priority inheritance protocol on mutex attributes: %s\n", strerror(ret));
		pthread_mutexattr_destroy(&mtxattr);
		return ret;
	}
#else /* _POSIX_THREAD_PRIO_INHERIT */
#warning Priority inheritance not supported!
#endif /* _POSIX_THREAD_PRIO_INHERIT */

	// TODO: set robustness on mutex attributes

	ret = pthread_mutex_init(mutex, &mtxattr);
	if(ret) {
		ERROR_OUT("Error creating thread locking mutex: %s\n", strerror(ret));
		pthread_mutexattr_destroy(&mtxattr);
		return ret;
	}

	ret = pthread_mutexattr_destroy(&mtxattr);
	if(ret) {
		ERROR_OUT("Error destroying mutex attributes: %s\n", strerror(ret));
		return ret;
	}

	return 0;
}
