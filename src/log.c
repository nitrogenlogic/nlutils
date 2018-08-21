/*
 * Logging-related functions.
 * Copyright (C)2012 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#include "nlutils.h"

/*
 * Like printf(), but prepends a timestamp and the name of the current thread
 * or process.  The output FILE will be locked using lockf() while the
 * timestamp, thread name, and message are written.  The thread name can be set
 * using nl_set_threadname().
 */
int __attribute__ ((__format__(__printf__, 1, 2))) nl_ptmf(const char *fmt, ...)
{
	va_list args;
	int ret;

	va_start(args, fmt);
	ret = nl_vfptmf(stdout, fmt, args);
	va_end(args);

	return ret;
}

/*
 * Like fprintf(), but prepends a timestamp and the name of the current thread
 * or process.  The output FILE will be locked using lockf() while the
 * timestamp, thread name, and message are written.  The thread name can be set
 * using nl_set_threadname().
 */
int __attribute__ ((__format__(__printf__, 2, 3))) nl_fptmf(FILE *out, const char *fmt, ...)
{
	va_list args;
	int ret;

	va_start(args, fmt);
	ret = nl_vfptmf(out, fmt, args);
	va_end(args);

	return ret;
}

/*
 * Like vfprintf(), but prepends a timestamp and the name of the current thread
 * or process.  The output FILE will be locked using lockf() while the
 * timestamp, thread name, and message are written.  The thread name can be set
 * using nl_set_threadname().
 */
int __attribute__ ((__format__(__printf__, 2, 0))) nl_vfptmf(FILE *out, const char *fmt, va_list args)
{
	struct timespec t;
	struct tm tm_result;
	char threadname[16];
	char buf[32];
	char zone[8];
	int ret;

	clock_gettime(CLOCK_REALTIME, &t);
	localtime_r(&t.tv_sec, &tm_result);
	strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_result);
	strftime(zone, sizeof(zone), "%z", &tm_result);

	nl_get_threadname(threadname);

	if(lockf(fileno(out), F_LOCK, 0)) {
		ERRNO_OUT("lockf lock failed");
	}
	ret = fprintf(out, "%s.%06ld %s - %s - ", buf, t.tv_nsec / 1000, zone, threadname);
	ret += vfprintf(out, fmt, args);
	if(lockf(fileno(out), F_ULOCK, 0)) {
		ERRNO_OUT("lockf unlock failed");
	}

	return ret;
}
