/*
 * Tests thread-related functions, such as thread renaming.
 * Copyright (C)2015 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "nlutils.h"

struct threadname_test {
	char *set;
	char *expect;
};

static struct threadname_test tests[] = {
	{
		.set = "",
		.expect = "",
	},
	{
		.set = "a name",
		.expect = "a name",
	},
	{
		.set = "__.!!\\/",
		.expect = "__.!!\\/",
	},
	{
		.set = "fifteen_chars!!",
		.expect = "fifteen_chars!!",
	},
	{
		.set = "sixteen_chars!!!",
		.expect = "sixteen_chars!!",
	},
	{
		.set = "watchdog_thread",
		.expect = "watchdog_thread",
	},
	{
		.set = "A very long name, much longer than fifteen characters.",
		.expect = "A very long nam",
	}
};

static int do_threadname_test(struct threadname_test *test)
{
	char name[32];
	int ret;

	ret = nl_set_threadname(test->set);
	if(ret) {
		ERROR_OUT("Error setting thread name to '%s': %s (%d).\n",
				test->set, strerror(ret), ret);
		return -1;
	}

	ret = nl_get_threadname(name);
	if(ret) {
		ERROR_OUT("Error getting thread name: %s (%d).\n",
				strerror(ret), ret);
		return -1;
	}

	if(strcmp(name, test->expect)) {
		ERROR_OUT("Error after setting thread name to '%s': got '%s', expected '%s'.\n",
				test->set, name, test->expect);
		return -1;
	}

	return 0;
}

#define NUM_THREADS 10

static void *thread_test_func(void *data)
{
	intptr_t id = (intptr_t)data;
	char name[16];

	nl_get_threadname(name);

	usleep(id * 100000);

	INFO_OUT("Thread %td (%s) started, sleeping for %td seconds.\n", id, name, id + 1);
	sleep(id + 1);
	INFO_OUT("Thread %td (%s) exiting.\n", id, name);

	return NULL;
}

static void *thread_name_func(void *data)
{
	intptr_t id = (intptr_t)data;
	char name[16];
	int ret;

	ret = nl_get_threadname(name);
	if(ret) {
		ERROR_OUT("Error getting name in multi-thread test: %d (%s)\n", ret, strerror(ret));
		exit(-1);
	}

	INFO_OUT("Name test thread %td: \"%s\".\n", id, name);

	return strdup(name);
}

static int mutex_tests()
{
	int mtx_types[] = { -1, PTHREAD_MUTEX_NORMAL, PTHREAD_MUTEX_RECURSIVE, PTHREAD_MUTEX_ERRORCHECK };
	char *mtx_names[] = { "default", "normal", "recursive", "errorcheck" };
	pthread_mutex_t mtx;
	size_t i;
	int ret;

	for(i = 0; i < ARRAY_SIZE(mtx_types); i++) {
		ret = nl_create_mutex(&mtx, mtx_types[i]);
		if(ret) {
			ERROR_OUT("Error creating %s mutex type: %s\n", mtx_names[i], strerror(ret));
			return -1;
		}

		ret = pthread_mutex_lock(&mtx);
		if(ret) {
			ERROR_OUT("Error locking %s mutex type: %s\n", mtx_names[i], strerror(ret));
			return -1;
		}

		// Make sure recursive and error checking mutexes were actually created
		if(mtx_types[i] == PTHREAD_MUTEX_RECURSIVE || mtx_types[i] == PTHREAD_MUTEX_ERRORCHECK) {
			ret = pthread_mutex_lock(&mtx);

			if(mtx_types[i] == PTHREAD_MUTEX_RECURSIVE) {
				if(ret) {
					ERROR_OUT("Recursive lock failed on recursive mutex: %s\n", strerror(ret));
					return -1;
				}
				ret = pthread_mutex_unlock(&mtx);
				if(ret) {
					ERROR_OUT("Recursive unlock failed on recursive mutex: %s\n", strerror(ret));
					return -1;
				}
			} else if(!ret) {
				ERROR_OUT("Expected failure on recursive error checking lock: %s\n", strerror(ret));
				return -1;
			}
		}

		ret = pthread_mutex_unlock(&mtx);
		if(ret) {
			ERROR_OUT("Error unlocking %s mutex type: %s\n", mtx_names[i], strerror(ret));
			return -1;
		}

		ret = pthread_mutex_destroy(&mtx);
		if(ret) {
			ERROR_OUT("Error destroying %s mutex type: %s\n", mtx_names[i], strerror(ret));
			return -1;
		}
	}

	return 0;
}

void thread_count_test(struct nl_thread *t, void *count)
{
	(void)t; // Deliberately unused parameter t
	(*(int *)count)++;
}

int main()
{
	struct nl_thread_ctx *ctx;
	struct nl_thread *info[NUM_THREADS];
	char buf[16];
	int ret;

	size_t i;

	// TODO: Test multiple thread contexts

	// Test thread creation and joining
	INFO_OUT("Testing thread creation and destruction.\n");
	DEBUG_OUT("Creating a thread context.\n");
	ctx = nl_create_thread_context();
	if(ctx == NULL) {
		ERRNO_OUT("Error allocating a thread context");
		return -1;
	}

	if(!pthread_equal(ctx->main_thread, pthread_self())) {
		ERROR_OUT("Thread context's main_thread differs from pthread_self().\n");
		nl_destroy_thread_context(ctx);
		return -1;
	}

	for(i = 0; i < NUM_THREADS; i++) {
		DEBUG_OUT("Creating thread %zu...\n", i);

		ret = nl_create_thread(ctx, NULL, thread_test_func, (void *)i, NULL, &info[i]);
		if(ret) {
			ERROR_OUT("Error creating thread %zu: %d (%s)\n", i, ret, strerror(ret));
			nl_destroy_thread_context(ctx);
			return -1;
		}
	}

	for(i = 0; i < NUM_THREADS; i++) {
		// TODO: Test SCHED_FIFO if running with sufficient capabilities
		DEBUG_OUT("Setting thread %zu scheduler to OTHER\n", i);

		ret = nl_set_thread_priority(info[i], SCHED_OTHER, 0);
		if(ret) {
			ERROR_OUT("Error setting thread %zu scheduler: %d (%s)\n", i, ret, strerror(ret));
			nl_destroy_thread_context(ctx);
			return -1;
		}
	}

	for(i = 2; i < NUM_THREADS; i += 2) {
		INFO_OUT("Joining thread %zu...\n", i);

		ret = nl_join_thread(info[i], NULL);
		if(ret) {
			ERROR_OUT("Error joining thread %zu: %d (%s)\n", i, ret, strerror(ret));
			nl_destroy_thread_context(ctx);
			return -1;
		}
	}

	DEBUG_OUT("Destroying thread context...\n");
	nl_destroy_thread_context(ctx);
	DEBUG_OUT("Thread context destroyed.\n");


	// Test main process thread renaming
	INFO_OUT("Testing simple thread naming.\n");
	for(i = 0; i < ARRAY_SIZE(tests); i++) {
		if(do_threadname_test(&tests[i])) {
			ERROR_OUT("Thread naming tests failed.\n");
			return -1;
		}
	}


	// Test independence of thread names
	INFO_OUT("Verifying thread names are independent.\n");
	ctx = nl_create_thread_context();

	for(i = 0; i < NUM_THREADS; i++) {
		snprintf(buf, sizeof(buf), "TN-%zu", i);
		ret = nl_create_thread(ctx, NULL, thread_name_func, (void *)i, buf, &info[i]);
		if(ret) {
			ERROR_OUT("Error creating name test thread %zu: %d (%s)\n", i, ret, strerror(ret));
			nl_destroy_thread_context(ctx);
			return -1;
		}
	}

	// Test thread iteration
	INFO_OUT("Testing thread iteration.\n");
	int iter_count = 0;
	nl_iterate_threads(ctx, 0, thread_count_test, &iter_count);
	if(iter_count != NUM_THREADS) {
		ERROR_OUT("Iteration should have counted %d threads, counted %d instead\n", NUM_THREADS, iter_count);
		nl_destroy_thread_context(ctx);
		return -1;
	}

	char *prev_name = strdup("");
	char *name;
	void *result;
	for(i = NUM_THREADS; i > 0; i--) {
		ret = nl_join_thread(info[i - 1], &result);
		if(ret) {
			ERROR_OUT("Error joining name test thread %zu: %d (%s)\n", i - 1, ret, strerror(ret));
			nl_destroy_thread_context(ctx);
			return -1;
		}

		name = (char *)result;
		if(!strcmp(prev_name, name)) {
			ERROR_OUT("Thread names match unexpectedly (%s == %s).\n", name, prev_name);
			nl_destroy_thread_context(ctx);
			return -1;
		}

		free(prev_name);
		prev_name = name;
	}
	free(prev_name);


	// Test mutex creation
	INFO_OUT("Testing mutexes.\n");
	if(mutex_tests()) {
		return -1;
	}

	nl_destroy_thread_context(ctx);

	return 0;
}
