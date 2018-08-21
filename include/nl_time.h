/*
 * Time-related functions.
 *
 * For timespec functions, inputs are typically in "away-from-zero" form,
 * meaning that negative timespecs greater than -1s should use a negative
 * value for tv_nsec, while negative timespecs larger than or equal to -1s
 * should use a negative value for tv_sec and a positive value for tv_nsec.
 *
 * Internally, timespecs are manipulated in "positive nanosecond" form,
 * meaning that tv_nsec is always positive, and always counts from tv_sec
 * toward positive infinity.  So, a timespec in positive nanosecond form that
 * is between -1 and 0 should use -1 for tv_sec, and a positive value for
 * tv_nsec between 0 and 1000000000.
 *
 * Copyright (C)2015 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#ifndef NLUTILS_TIME_H_
#define NLUTILS_TIME_H_

#include <time.h>

/*
 * Evaluates to true if a >= b, where a and b are both positive, or in positive
 * nanosecond form (evaluates a and b multiple times).  Only works for
 * normalized timespecs.
 */
#define NL_TIMESPEC_GTE(a, b) ( (a).tv_sec > (b).tv_sec || ((a).tv_sec == (b).tv_sec && (a).tv_nsec >= (b).tv_nsec) )


/*
 * Converts a timespec from away-from-zero form (more natural for a human to
 * write) to positive nanosecond form (easier for arithmetic).
 */
NLUTILS_INLINE struct timespec nl_timespec_to_pos(const struct timespec ts)
{
	if(ts.tv_sec < 0) {
		struct timespec a = {
			ts.tv_sec - 1,
			1000000000 - ts.tv_nsec
		};

		return a;
	} else if(ts.tv_nsec < 0) {
		struct timespec b = {
			ts.tv_sec - 1,
			1000000000 + ts.tv_nsec
		};

		return b;
	} else {
		struct timespec c = {
			ts.tv_sec,
			ts.tv_nsec
		};

		return c;
	}
}

/*
 * Converts a timespec from positive nanosecond form to away-from-zero form.
 */
NLUTILS_INLINE struct timespec nl_timespec_from_pos(struct timespec ts)
{
	if(ts.tv_nsec) {
		if(ts.tv_sec == -1) {
			ts.tv_sec = 0;
			ts.tv_nsec -= 1000000000;
		} else if(ts.tv_sec < -1) {
			ts.tv_sec += 1;
			ts.tv_nsec = 1000000000 - ts.tv_nsec;
		}
	}

	return ts;
}

/*
 * Normalizes absolute tv_nsec values to be between 0 and 1000000000, accepting
 * and returning away-from-zero form.
 */
NLUTILS_INLINE struct timespec nl_normalize_timespec(struct timespec ts)
{
	ts = nl_timespec_to_pos(ts);

	while(ts.tv_nsec < 0) {
		ts.tv_sec--;
		ts.tv_nsec += 1000000000;
	}
	while(ts.tv_nsec >= 1000000000) {
		ts.tv_sec++;
		ts.tv_nsec -= 1000000000;
	}

	ts = nl_timespec_from_pos(ts);

	return ts;
}

/*
 * Negates a struct timespec, with tv_nsec always positive unless tv_sec is 0,
 * and tv_nsec counting downward for negative values.
 */
NLUTILS_INLINE struct timespec nl_negate_timespec(const struct timespec ts)
{
	if(ts.tv_sec == 0) {
		struct timespec a = { 0, -ts.tv_nsec };
		return a;
	} else {
		struct timespec b = { -ts.tv_sec, ts.tv_nsec };
		return b;
	}
}

/*
 * Returns the sum of timespecs a and b in away-from-zero form.  Does not
 * handle integer overflow of seconds or nanoseconds, but nanosecond overflow
 * will not occur if both timespecs are normalized with absolute nanoseconds
 * less than one billion.
 */
NLUTILS_INLINE struct timespec nl_add_timespec(const struct timespec a, const struct timespec b)
{
	struct timespec ts;

	// Convert to additive nanosecond form first (similar to two's complement)
	struct timespec a_pos = nl_timespec_to_pos(a), b_pos = nl_timespec_to_pos(b);

	ts.tv_sec = a_pos.tv_sec + b_pos.tv_sec;
	ts.tv_nsec = a_pos.tv_nsec + b_pos.tv_nsec;

	int sign = ts.tv_nsec < 0 ? -1 : 1;
	int offs = sign * 1000000000;
	while(ts.tv_nsec >= 1000000000 || ts.tv_nsec < 0) {
		ts.tv_sec += sign;
		ts.tv_nsec -= offs;
	}

	// Convert back to away-from-zero nanosecond form
	return nl_timespec_from_pos(ts);
}

/*
 * Returns timespec a minus timespec b in away-from-zero form.  Does not handle
 * integer overflow of seconds or nanoseconds, but nanosecond overflow will not
 * occur if both timespecs are normalized with absolute nanoseconds less than
 * one billion.
 */
NLUTILS_INLINE struct timespec nl_sub_timespec(const struct timespec a, const struct timespec b)
{
	return nl_add_timespec(a, nl_negate_timespec(b));
}

/*
 * Returns a value less than, equal to, or greater than zero if a is less than,
 * equal to, or greater than b, respectively.  Both a and b should be in
 * away-from-zero form.  Only works correctly for normalized timespecs.
 */
NLUTILS_INLINE int nl_compare_timespec(const struct timespec a, const struct timespec b)
{
	if(a.tv_sec < b.tv_sec) {
		return -1;
	}
	if(a.tv_sec == b.tv_sec) {
		if(a.tv_nsec == b.tv_nsec) {
			return 0;
		}
		if(a.tv_sec < 0) {
			if(a.tv_nsec > b.tv_nsec) {
				return -1;
			}
		} else {
			if(a.tv_nsec < b.tv_nsec) {
				return -1;
			}
		}
	}

	return 1;
}

/*
 * Stores the current time according to clock_id, plus from_now, in target.
 * Returns 0 on success, an errno-like value on error.
 */
NLUTILS_INLINE int nl_clock_fromnow(const clockid_t clock_id, struct timespec * const target, const struct timespec from_now)
{
	struct timespec ts;
	int ret;

	if(!target) {
		return EFAULT;
	}

	ret = clock_gettime(clock_id, &ts);
	if(ret) {
		return errno;
	}

	*target = nl_add_timespec(ts, from_now);

	return 0;
}

/*
 * Sleeps for at least the specified number of microseconds, which may be
 * greater than 1000000.  If the underlying sleep call is interrupted, it will
 * be resumed.
 */
void nl_usleep(unsigned long usecs);


#endif /* NLUTILS_TIME_H_ */
