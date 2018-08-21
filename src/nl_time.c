/*
 * Time-related functions.
 * Copyright (C)2015 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <time.h>

#include "nlutils.h"


/*
 * Sleeps for at least the specified number of microseconds, which may be
 * greater than 1000000.  If the underlying sleep call is interrupted, it will
 * be resumed.
 */
void nl_usleep(unsigned long usecs)
{
	struct timespec ts;

	if(usecs >= 1000000) {
		ts.tv_sec = usecs / 1000000ul;
		ts.tv_nsec = (usecs % 1000000ul) * 1000;
	} else {
		ts.tv_sec = 0;
		ts.tv_nsec = usecs * 1000;
	}

	while(nanosleep(&ts, &ts) == -1 && errno == EINTR) {
		// Do nothing while sleeping 
	}
}
