/*
 * Logging-related functions.
 * Copyright (C)2012 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#ifndef NLUTILS_LOG_H_
#define NLUTILS_LOG_H_

/*
 * Like printf(), but prepends a timestamp and the name of the current thread
 * or process.  The output FILE will be locked using lockf() while the
 * timestamp, thread name, and message are written.  The thread name can be set
 * using nl_set_threadname().
 */
int __attribute__ ((__format__(__printf__, 1, 2))) nl_ptmf(const char *fmt, ...);

/*
 * Like fprintf(), but prepends a timestamp and the name of the current thread
 * or process.  The output FILE will be locked using lockf() while the
 * timestamp, thread name, and message are written.  The thread name can be set
 * using nl_set_threadname().
 */
int __attribute__ ((__format__(__printf__, 2, 3))) nl_fptmf(FILE *out, const char *fmt, ...);

/*
 * Like vfprintf(), but prepends a timestamp and the name of the current thread
 * or process.  The output FILE will be locked using lockf() while the
 * timestamp, thread name, and message are written.  The thread name can be set
 * using nl_set_threadname().
 */
int __attribute__ ((__format__(__printf__, 2, 0))) nl_vfptmf(FILE *out, const char *format, va_list args);

#endif /* NLUTILS_LOG_H_ */
