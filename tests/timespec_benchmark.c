/*
 * Tests speed of different approaches to adding and subtracting timespecs.
 * Copyright (C)2015 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdio.h>
#include <time.h>

#include "nlutils.h"

// Returns integer nanoseconds for the given normalized timespec
// TODO: Maybe move this method to library or header
static inline int64_t ts_to_nano(struct timespec ts)
{
	return ts.tv_sec * 1000000000 + (ts.tv_sec >= 0 ? ts.tv_nsec : -ts.tv_nsec);
}

// Returns a normalized timespec for the given integer nanoseconds
// TODO: Maybe move this method to library or header
static inline struct timespec nano_to_ts(int64_t nano)
{
	return (struct timespec){
		.tv_sec = nano / 1000000000,
		.tv_nsec = nano > -1000000000 ? nano % 1000000000 : -nano % 1000000000
	};
}


static int64_t clock_getnano(void)
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return ts_to_nano(now);
}

static inline void add_timespec_unsafe(struct timespec *ts1, const struct timespec *ts2)
{
	ts1->tv_sec += ts2->tv_sec;
	ts1->tv_nsec += ts2->tv_nsec;

	// Since this will rarely exceed 2 billion, it is probably faster to
	// iteratively subtract than to do a full division and remainder.
	while(ts1->tv_nsec >= 1000000000) {
		ts1->tv_sec++;
		ts1->tv_nsec -= 1000000000;
	}
}

static inline struct timespec add_timespec_unsafe_direct(struct timespec ts1, struct timespec ts2)
{
	struct timespec ts = {
		.tv_sec = ts1.tv_sec + ts2.tv_sec,
		.tv_nsec = ts1.tv_nsec + ts2.tv_nsec
	};

	while(ts.tv_nsec >= 1000000000) {
		ts.tv_sec++;
		ts.tv_nsec -= 1000000000;
	}

	return ts;
}

static inline void sub_timespec_unsafe(struct timespec *ts1, const struct timespec *ts2)
{
	ts1->tv_sec -= ts2->tv_sec;
	ts1->tv_nsec -= ts2->tv_nsec;

	while(ts1->tv_nsec < 0) {
		ts1->tv_sec--;
		ts1->tv_nsec += 1000000000;
	}
}

static inline struct timespec nano_add(const struct timespec ts1, const struct timespec ts2)
{
	return nano_to_ts(ts_to_nano(ts1) + ts_to_nano(ts2));
}

static inline struct timespec nano_sub(const struct timespec ts1, const struct timespec ts2)
{
	return nano_to_ts(ts_to_nano(ts1) - ts_to_nano(ts2));
}


#define TIME_LIMIT 5000000000 // five seconds

int main(void)
{
	volatile struct timespec result;
	int64_t start;
	int64_t elapsed;
	double seconds;
	size_t iterations;
	int i;

	INFO_OUT("Testing nl_add_timespec\n");
	for(start = clock_getnano(), iterations = 0, elapsed = 0; elapsed < TIME_LIMIT; elapsed = clock_getnano() - start) {
		for(i = 1; i < 10000000; i++) {
			iterations++;
			result = nl_add_timespec((struct timespec){.tv_sec = i}, (struct timespec){.tv_nsec = i});
			if(result.tv_sec != i || result.tv_nsec != i) {
				ERROR_OUT("Result failed\n");
				return -1;
			}
		}
	}
	seconds = (double)elapsed / 1000000000.0;
	INFO_OUT("  %zd iterations in %.3lfs: %.3lf per second\n", iterations, seconds, (double)iterations / seconds);

	INFO_OUT("Testing add_timespec_unsafe\n");
	for(start = clock_getnano(), iterations = 0, elapsed = 0; elapsed < TIME_LIMIT; elapsed = clock_getnano() - start) {
		for(i = 1; i < 10000000; i++) {
			iterations++;
			result = (struct timespec){.tv_sec = i};
			add_timespec_unsafe((struct timespec *)&result, &(struct timespec){.tv_nsec = i});
			if(result.tv_sec != i || result.tv_nsec != i) {
				ERROR_OUT("Result failed\n");
				return -1;
			}
		}
	}
	seconds = (double)elapsed / 1000000000.0;
	INFO_OUT("  %zd iterations in %.3lfs: %.3lf per second\n", iterations, seconds, (double)iterations / seconds);

	INFO_OUT("Testing add_timespec_unsafe_direct\n");
	for(start = clock_getnano(), iterations = 0, elapsed = 0; elapsed < TIME_LIMIT; elapsed = clock_getnano() - start) {
		for(i = 1; i < 10000000; i++) {
			iterations++;
			result = add_timespec_unsafe_direct((struct timespec){.tv_sec = i}, (struct timespec){.tv_nsec = i});
			if(result.tv_sec != i || result.tv_nsec != i) {
				ERROR_OUT("Result failed\n");
				return -1;
			}
		}
	}
	seconds = (double)elapsed / 1000000000.0;
	INFO_OUT("  %zd iterations in %.3lfs: %.3lf per second\n", iterations, seconds, (double)iterations / seconds);

	INFO_OUT("Testing nano_add\n");
	for(start = clock_getnano(), iterations = 0, elapsed = 0; elapsed < TIME_LIMIT; elapsed = clock_getnano() - start) {
		for(i = 1; i < 10000000; i++) {
			iterations++;
			result = nano_add((struct timespec){.tv_sec = i}, (struct timespec){.tv_nsec = i});
			if(result.tv_sec != i || result.tv_nsec != i) {
				ERROR_OUT("Result failed\n");
				return -1;
			}
		}
	}
	seconds = (double)elapsed / 1000000000.0;
	INFO_OUT("  %zd iterations in %.3lfs: %.3lf per second\n", iterations, seconds, (double)iterations / seconds);

	INFO_OUT("Testing nl_sub_timespec\n");
	for(start = clock_getnano(), iterations = 0, elapsed = 0; elapsed < TIME_LIMIT; elapsed = clock_getnano() - start) {
		for(i = 1; i < 10000000; i++) {
			iterations++;
			result = nl_sub_timespec((struct timespec){.tv_sec = i + 1}, (struct timespec){.tv_nsec = i});
			if(result.tv_sec != i || result.tv_nsec != (1000000000 - i)) {
				ERROR_OUT("Result %d failed with %ld.%09ld\n", i, result.tv_sec, result.tv_nsec);
				return -1;
			}
		}
	}
	seconds = (double)elapsed / 1000000000.0;
	INFO_OUT("  %zd iterations in %.3lfs: %.3lf per second\n", iterations, seconds, (double)iterations / seconds);

	INFO_OUT("Testing sub_timespec_unsafe\n");
	for(start = clock_getnano(), iterations = 0, elapsed = 0; elapsed < TIME_LIMIT; elapsed = clock_getnano() - start) {
		for(i = 1; i < 10000000; i++) {
			iterations++;
			result = (struct timespec){.tv_sec = i + 1};
			sub_timespec_unsafe((struct timespec *)&result, &(struct timespec){.tv_nsec = i});
			if(result.tv_sec != i || result.tv_nsec != (1000000000 - i)) {
				ERROR_OUT("Result %d failed with %ld.%09ld\n", i, result.tv_sec, result.tv_nsec);
				return -1;
			}
		}
	}
	seconds = (double)elapsed / 1000000000.0;
	INFO_OUT("  %zd iterations in %.3lfs: %.3lf per second\n", iterations, seconds, (double)iterations / seconds);

	INFO_OUT("Testing nano_sub\n");
	for(start = clock_getnano(), iterations = 0, elapsed = 0; elapsed < TIME_LIMIT; elapsed = clock_getnano() - start) {
		for(i = 1; i < 10000000; i++) {
			iterations++;
			result = nano_sub((struct timespec){.tv_sec = i + 1}, (struct timespec){.tv_nsec = i});
			if(result.tv_sec != i || result.tv_nsec != (1000000000 - i)) {
				ERROR_OUT("Result %d failed with %ld.%09ld\n", i, result.tv_sec, result.tv_nsec);
				return -1;
			}
		}
	}
	seconds = (double)elapsed / 1000000000.0;
	INFO_OUT("  %zd iterations in %.3lfs: %.3lf per second\n", iterations, seconds, (double)iterations / seconds);

	return 0;
}
