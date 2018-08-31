/*
 * Thread-related functions.
 * Copyright (C)2015, 2018 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#ifndef NLUTILS_THREAD_H_
#define NLUTILS_THREAD_H_

#include <pthread.h>

struct nl_thread_ctx;
struct nl_thread;

/*
 * Callback function for nl_iterate_threads().
 */
typedef void (*nl_thread_iterator)(struct nl_thread *thread, void *cb_data);

/*
 * Information about a single tracked thread.
 */
struct nl_thread {
	struct nl_thread_ctx *ctx; // Tracking context that owns the thread

	pthread_t thread;	// Thread ID
	char name[16];		// Thread name at time of creation

	void *(*func)(void *);	// Thread function
	void *data;		// User-supplied thread parameter

	struct nl_thread *next;	// Next info in linked list
};

/*
 * A context in which tracked threads may be created.
 */
struct nl_thread_ctx {
	pthread_t main_thread;		// The thread that created this context
	pthread_mutex_t lock;		// Protects access to thread list
	struct nl_thread *threads;	// Linked list of threads
};


/*
 * Sets the name of the current thread or process, truncating to at most 15
 * bytes plus NUL.  Returns 0 on success, or a positive errno-style error code
 * on failure.
 */
int nl_set_threadname(char *name);

/*
 * Stores the name of the current thread in the given buffer, which must be
 * able to hold at least 16 bytes, including NUL.  A 16-byte name will be
 * truncated to 15 bytes plus NUL.  Returns 0 on success, or a positive
 * errno-style error code on failure.
 */
int nl_get_threadname(char *name);

/*
 * Creates an empty thread context for use with nl_create_thread() and related
 * functions.  Returns NULL on error.
 */
struct nl_thread_ctx *nl_create_thread_context();

/*
 * Joins all threads on the given context, then frees the context's memory.
 */
void nl_destroy_thread_context(struct nl_thread_ctx *ctx);

/*
 * Calls pthread_create to create a thread, storing the pthread_t handle in a
 * new entry in the given thread context.  A pointer to the thread's info
 * structure is stored in **info if it is not NULL (note that the pthread_t
 * handle for the thread is returned in **info).  If *name is not NULL, then
 * the thread's name will be set to name immediately after creation.  Returns 0
 * on success, a positive errno-like value on error.
 */
int nl_create_thread(struct nl_thread_ctx *ctx, const pthread_attr_t *attr, void *(*func)(void *), void *data, char *name, struct nl_thread **info);

/*
 * Attempts to join the given thread, then removes it from its thread context.
 * The thread info will be removed from the context and its memory freed
 * regardless of whether joining succeeds.  The return value of the thread's
 * main function will be stored in **result if it is not NULL (see the manual
 * page for pthread_join()).  Returns 0 on success, an errno-like value on
 * error.
 */
int nl_join_thread(struct nl_thread *info, void **result);

/*
 * Sets the scheduling class (e.g. SCHED_OTHER, SCHED_FIFO) and priority of the
 * given thread, or the current thread if thread is NULL.  Note that this is
 * not the same thing as niceness.  Returns 0 on success, an errno-like value
 * on error.
 */
int nl_set_thread_priority(struct nl_thread *thread, int sched_class, int prio);

/*
 * Calls the given callback for each thread that was created in the given
 * threading context (thus the main thread that created the context is
 * excluded).  If lock_timeout_us is greater than zero, then iteration will
 * proceed anyway after approximately lock_timeout_us microseconds even if the
 * thread list lock cannot be obtained.  The list of threads should not be
 * modified by the callback (no creation or joining of threads).
 */
void nl_iterate_threads(struct nl_thread_ctx *ctx, int lock_timeout_us, nl_thread_iterator cb, void *cb_data);

/*
 * Sends the given signal to all the threads in the given context, excluding
 * the main thread that created the context and the current thread (if it's one
 * of the threads in the context).  Waits 25ms between threads.
 */
void nl_signal_threads(struct nl_thread_ctx *ctx, int signum);

/*
 * Creates a priority inheritance mutex (PTHREAD_PRIO_INHERIT) in *mutex.  Pass
 * PTHREAD_MUTEX_NORMAL for type to create a standard mutex.  For a recursive
 * mutex, pass -1 or PTHREAD_MUTEX_RECURSIVE.  For an error checking mutex,
 * pass PTHREAD_MUTEX_ERRORCHECK.  Returns 0 on success, or an ERRNO-like value
 * on error.
 */
int nl_create_mutex(pthread_mutex_t *mutex, int type);


#endif /* NLUTILS_THREAD_H_ */
