/*
 * Tests time functions.
 * Copyright (C)2015 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "nlutils.h"

struct operator_test {
	char *name;
	struct timespec a; // First input parameter
	struct timespec b; // Second input parameter
	struct timespec add; // Expected result from addition
	struct timespec sub; // Expected result from subtraction
	struct timespec neg_b; // Expected result from negation of B (only checked if nonzero)
	int compare; // Expected result from comparison
};

static struct operator_test op_tests[] = {
	{
		.name = "All zeros",
	},
	{
		.name = "Normalize +ns (1000000001)",
		.a = { .tv_sec = 0, .tv_nsec = 1000000001 },
		.add = { .tv_sec = 1, .tv_nsec = 1 },
		.sub = { .tv_sec = 1, .tv_nsec = 1 },
		.compare = 1,
	},
	{
		.name = "Don't normalize +ns (999999999)",
		.a = { .tv_sec = 0, .tv_nsec = 999999999 },
		.add = { .tv_sec = 0, .tv_nsec = 999999999 },
		.sub = { .tv_sec = 0, .tv_nsec = 999999999 },
		.compare = 1,
	},
	{
		.name = "Don't normalize -ns (-999999999)",
		.a = { .tv_sec = 0, .tv_nsec = -999999999 },
		.add = { .tv_sec = 0, .tv_nsec = -999999999 },
		.sub = { .tv_sec = 0, .tv_nsec = -999999999 },
		.compare = -1,
	},
	{
		.name = "Normalize -ns (-2000000002)",
		.a = { .tv_sec = 0, .tv_nsec = -2000000002 },
		.add = { .tv_sec = -2, .tv_nsec = 2 },
		.sub = { .tv_sec = -2, .tv_nsec = 2 },
		.compare = -1,
	},
	{
		.name = "Normalize +s+ns (1|2000000003)",
		.a = { .tv_sec = 1, .tv_nsec = 2000000003 },
		.add = { .tv_sec = 3, .tv_nsec = 3 },
		.sub = { .tv_sec = 3, .tv_nsec = 3 },
		.compare = 1,
	},
	{
		.name = "Normalize +s-ns (1|-2000000001)",
		.a = { .tv_sec = 1, .tv_nsec = -2000000001 },
		.add = { .tv_sec = -1, .tv_nsec = 1 },
		.sub = { .tv_sec = -1, .tv_nsec = 1 },
		.compare = -1,
	},
	{
		.name = "Normalize -s+ns (-1|1000000000)",
		.a = { .tv_sec = -1, .tv_nsec = 1000000000 },
		.add = { .tv_sec = -2 },
		.sub = { .tv_sec = -2 },
		.compare = -1,
	},
	{
		.name = "Normalize -s-ns (-1|-000000001)",
		.a = { .tv_sec = -1, .tv_nsec = -1 },
		.add = { .tv_sec = 0, .tv_nsec = -999999999 },
		.sub = { .tv_sec = 0, .tv_nsec = -999999999 },
		.compare = -1,
	},

	{
		.name = "Negate +ns",
		.b = { .tv_nsec = 1 },
		.add = { .tv_nsec = 1 },
		.sub = { .tv_nsec = -1 },
		.neg_b = { .tv_nsec = -1 },
		.compare = -1,
	},
	{
		.name = "Negate +bigns",
		.b = { .tv_nsec = 999999999 },
		.add = { .tv_nsec = 999999999 },
		.sub = { .tv_nsec = -999999999 },
		.neg_b = { .tv_nsec = -999999999 },
		.compare = -1,
	},
	{
		.name = "Negate +s+ns",
		.b = { .tv_sec = 1, .tv_nsec = 1 },
		.add = { .tv_sec = 1, .tv_nsec = 1 },
		.sub = { .tv_sec = -1, .tv_nsec = 1 },
		.neg_b = { .tv_sec = -1, .tv_nsec = 1 },
		.compare = -1,
	},
	{
		.name = "Negate +s+bigns",
		.b = { .tv_sec = 1, .tv_nsec = 999999999 },
		.add = { .tv_sec = 1, .tv_nsec = 999999999 },
		.sub = { .tv_sec = -1, .tv_nsec = 999999999 },
		.neg_b = { .tv_sec = -1, .tv_nsec = 999999999 },
		.compare = -1,
	},
	{
		.name = "Negate -ns",
		.b = { .tv_nsec = -1 },
		.add = { .tv_nsec = -1 },
		.sub = { .tv_nsec = 1 },
		.neg_b = { .tv_nsec = 1 },
		.compare = 1,
	},
	{
		.name = "Negate -bigns",
		.b = { .tv_nsec = -999999999 },
		.add = { .tv_nsec = -999999999 },
		.sub = { .tv_nsec = 999999999 },
		.neg_b = { .tv_nsec = 999999999 },
		.compare = 1,
	},
	{
		.name = "Negate -s+ns",
		.b = { .tv_sec = -1, .tv_nsec = 1 },
		.add = { .tv_sec = -1, .tv_nsec = 1 },
		.sub = { .tv_sec = 1, .tv_nsec = 1 },
		.neg_b = { .tv_sec = 1, .tv_nsec = 1 },
		.compare = 1,
	},
	{
		.name = "Negate -s+bigns",
		.b = { .tv_sec = -1, .tv_nsec = 999999999 },
		.add = { .tv_sec = -1, .tv_nsec = 999999999 },
		.sub = { .tv_sec = 1, .tv_nsec = 999999999 },
		.neg_b = { .tv_sec = 1, .tv_nsec = 999999999 },
		.compare = 1,
	},

	{
		.name = "One and zero",
		.a = { .tv_sec = 1 },
		.b = { .tv_sec = 0 },
		.add = { .tv_sec = 1 },
		.sub = { .tv_sec = 1 },
		.compare = 1,
	},
	{
		.name = "Zero and one",
		.a = { .tv_sec = 0 },
		.b = { .tv_sec = 1 },
		.add = { .tv_sec = 1 },
		.sub = { .tv_sec = -1 },
		.compare = -1,
	},
	{
		.name = "Equal positive seconds",
		.a = { .tv_sec = 4 },
		.b = { .tv_sec = 4 },
		.add = { .tv_sec = 8 },
		.sub = { .tv_sec = 0 },
		.compare = 0,
	},
	{
		.name = "Equal negative seconds",
		.a = { .tv_sec = -4 },
		.b = { .tv_sec = -4 },
		.add = { .tv_sec = -8 },
		.sub = { .tv_sec = 0 },
		.compare = 0,
	},
	{
		.name = "Equal with nanoseconds",
		.a = { .tv_sec = 1, .tv_nsec = 1111 },
		.b = { .tv_sec = 1, .tv_nsec = 1111 },
		.add = { .tv_sec = 2, .tv_nsec = 2222 },
		.sub = { .tv_sec = 0, .tv_nsec = 0 },
		.compare = 0,
	},
	{
		.name = "Positive seconds",
		.a = { .tv_sec = 3 },
		.b = { .tv_sec = 2 },
		.add = { .tv_sec = 5 },
		.sub = { .tv_sec = 1 },
		.compare = 1,
	},
	{
		.name = "Negative seconds",
		.a = { .tv_sec = -2 },
		.b = { .tv_sec = -1 },
		.add = { .tv_sec = -3 },
		.sub = { .tv_sec = -1 },
		.compare = -1,
	},
	{
		.name = "Positive and negative",
		.a = { .tv_sec = 5 },
		.b = { .tv_sec = -3 },
		.add = { .tv_sec = 2 },
		.sub = { .tv_sec = 8 },
		.compare = 1,
	},
	{
		.name = "Negative and positive",
		.a = { .tv_sec = -5 },
		.b = { .tv_sec = 3 },
		.add = { .tv_sec = -2 },
		.sub = { .tv_sec = -8 },
		.compare = -1,
	},
	{
		.name = "Positive small mixed",
		.a = { .tv_sec = 2, .tv_nsec = 12345 },
		.b = { .tv_sec = 1, .tv_nsec = 65432 },
		.add = { .tv_sec = 3, .tv_nsec = 77777 },
		.sub = { .tv_sec = 0, .tv_nsec = 999946913 },
		.compare = 1,
	},
	{
		.name = "Positive large mixed",
		.a = { .tv_sec = 1000000001, .tv_nsec = 999999999 },
		.b = { .tv_sec = 1000000000, .tv_nsec = 1 },
		.add = { .tv_sec = 2000000002, .tv_nsec = 0 },
		.sub = { .tv_sec = 1, .tv_nsec = 999999998 },
		.compare = 1,
	},
	{
		.name = "Small nanoseconds",
		.a = { .tv_nsec = 202 },
		.b = { .tv_nsec = 102 },
		.add = { .tv_nsec = 304 },
		.sub = { .tv_nsec = 100 },
		.compare = 1,
	},
	{
		.name = "Small negative nanoseconds",
		.a = { .tv_nsec = -303 },
		.b = { .tv_nsec = -103 },
		.add = { .tv_nsec = -406 },
		.sub = { .tv_nsec = -200 },
		.compare = -1,
	},
	{
		.name = "Large nanoseconds",
		.a = { .tv_nsec = 999999234 },
		.b = { .tv_nsec = 999999000 },
		.add = { .tv_sec = 1, .tv_nsec = 999998234 },
		.sub = { .tv_nsec = 234 },
		.compare = 1,
	},
	{
		.name = "Positive nanoseconds with negative result",
		.a = { .tv_nsec = 5000 },
		.b = { .tv_nsec = 5001 },
		.add = { .tv_sec = 0, .tv_nsec = 10001 },
		.sub = { .tv_sec = 0, .tv_nsec = -1 },
		.compare = -1,
	},
	{
		.name = "Close negative greater",
		.a = { .tv_sec = -1, .tv_nsec = 100000 },
		.b = { .tv_sec = -1, .tv_nsec = 100001 },
		.add = { .tv_sec = -2, .tv_nsec = 200001 },
		.sub = { .tv_sec = 0, .tv_nsec = 1 },
		.compare = 1,
	},
	{
		.name = "Close negative equal",
		.a = { .tv_sec = -1, .tv_nsec = 100100 },
		.b = { .tv_sec = -1, .tv_nsec = 100100 },
		.add = { .tv_sec = -2, .tv_nsec = 200200 },
		.sub = { .tv_sec = 0, .tv_nsec = 0 },
		.compare = 0,
	},
	{
		.name = "Close negative lesser",
		.a = { .tv_sec = -1, .tv_nsec = 100003 },
		.b = { .tv_sec = -1, .tv_nsec = 100002 },
		.add = { .tv_sec = -2, .tv_nsec = 200005 },
		.sub = { .tv_sec = 0, .tv_nsec = -1 },
		.compare = -1,
	},
	{
		.name = "Negative carry lesser",
		.a = { .tv_sec = -1, .tv_nsec = 1 },
		.b = { .tv_sec = 0, .tv_nsec = -999999999 },
		.add = { .tv_sec = -2, .tv_nsec = 0 },
		.sub = { .tv_sec = 0, .tv_nsec = -2 },
		.compare = -1,
	},
	{
		.name = "Negative carry greater",
		.a = { .tv_sec = 0, .tv_nsec = -999999999 },
		.b = { .tv_sec = -1, .tv_nsec = 1 },
		.add = { .tv_sec = -2, .tv_nsec = 0 },
		.sub = { .tv_sec = 0, .tv_nsec = 2 },
		.compare = 1,
	},
	{
		.name = "Large negative",
		.a = { .tv_sec = -500000, .tv_nsec = 999999111 },
		.b = { .tv_sec = -1, .tv_nsec = 1000 },
		.add = { .tv_sec = -500002, .tv_nsec = 111 },
		.sub = { .tv_sec = -499999, .tv_nsec = 999998111 },
		.compare = -1,
	},
	{
		.name = "Large positive with small negative result",
		.a = { .tv_sec = 2, .tv_nsec = 999999000 },
		.b = { .tv_sec = 2, .tv_nsec = 999999011 },
		.add = { .tv_sec = 5, .tv_nsec = 999998011 },
		.sub = { .tv_sec = 0, .tv_nsec = -11 },
		.compare = -1,
	},
	{
		.name = "Large negative with small negative result",
		.a = { .tv_sec = -2, .tv_nsec = 999999000 },
		.b = { .tv_sec = -2, .tv_nsec = 999998999 },
		.add = { .tv_sec = -5, .tv_nsec = 999997999 },
		.sub = { .tv_sec = 0, .tv_nsec = -1 },
		.compare = -1,
	},
};

// Displays an error and returns nonzero if expected != actual
static int check_timetest(struct timespec expected, struct timespec actual, char *opname, char *testname)
{
	if(memcmp(&expected, &actual, sizeof(struct timespec))) {
		ERROR_OUT("Expected %ld.%09ld, got %ld.%09ld for %s on %s\n",
				expected.tv_sec, expected.tv_nsec,
				actual.tv_sec, actual.tv_nsec,
				opname, testname);
		return -1;
	}

	return 0;
}

// Displays an error and returns nonzero if expected != actual
static int check_compare(int expected, int actual, char *compname, char *testname)
{
	if(expected != actual) {
		ERROR_OUT("Expected %d, got %d for %s comparison on %s\n",
				expected, actual, compname, testname);
		return -1;
	}

	return 0;
}

// Compares to within one roundmul-th
static int check_convert_dbl(double expected, double actual, int roundmul, char *convname, char *testname)
{
	if (round(expected * roundmul) != round(actual * roundmul)) {
		ERROR_OUT("Expected %f, got %f for %s conversion on %s\n",
				expected, actual, convname, testname);
		return -1;
	}

	return 0;
}

// Returns integer nanoseconds for the given normalized timespec
static int64_t ts_to_nano(struct timespec ts)
{
	return ts.tv_sec * (int64_t)1000000000 + (ts.tv_sec >= 0 ? ts.tv_nsec : -ts.tv_nsec);
}

// Returns a normalized timespec for the given integer nanoseconds
static struct timespec nano_to_ts(int64_t nano)
{
	return (struct timespec){
		.tv_sec = nano / 1000000000,
		.tv_nsec = nano > -1000000000 ? nano % 1000000000 : -nano % 1000000000
	};
}

// Returns 'A' if timespec <= -1, 'B' if -1 < timespec < 0, 'C' if 0 <=
// timespec < 1, 'D' if 1 <= timespec, '!' if timespec is invalid
static char ts_zone(struct timespec ts)
{
	if(ts.tv_sec <= -1) {
		return 'A';
	}
	if(ts.tv_sec == 0 && ts.tv_nsec < 0) {
		return 'B';
	}
	if(ts.tv_sec == 0 && ts.tv_nsec >= 0) {
		return 'C';
	}
	if(ts.tv_sec >= 1) {
		return 'D';
	}

	return '!';
}

static int failures = 0;
static int tests = 0;

// Runs numeric timespec operator tests
static void numeric_operators(void)
{
	const int64_t zero_list[] = { -999999999, -999999998, -2, -1, 0, 1, 2, 999999998, 999999999 };
	const int64_t nonzero_list[] = { 0, 1, 2, 999999998, 999999999 };
	struct timespec ts;
	char testname[128];
	int comp;

	INFO_OUT("Numeric tests of timespec operators\n");
	for(int64_t sec_a = -2; sec_a <= 2; sec_a++) {
		int max_a = sec_a == 0 ? ARRAY_SIZE(zero_list) : ARRAY_SIZE(nonzero_list);
		const int64_t *list_a = sec_a == 0 ? zero_list : nonzero_list;

		for(int64_t sec_b = -2; sec_b <= 2; sec_b++) {
			int max_b = sec_b == 0 ? ARRAY_SIZE(zero_list) : ARRAY_SIZE(nonzero_list);
			const int64_t *list_b = sec_b == 0 ? zero_list : nonzero_list;

			for(int idx_a = 0; idx_a < max_a; idx_a++) {
				int64_t nsec_a = list_a[idx_a];

				for(int idx_b = 0; idx_b < max_b; idx_b++) {
					int64_t nsec_b = list_b[idx_b];

					struct timespec a = { .tv_sec = sec_a, .tv_nsec = nsec_a };
					struct timespec b = { .tv_sec = sec_b, .tv_nsec = nsec_b };
					int64_t nano;

					snprintf(testname, sizeof(testname),
							"%"PRId64".%09"PRId64" and %"PRId64".%09"PRId64" (%c, %c)",
							sec_a, nsec_a, sec_b, nsec_b, ts_zone(a), ts_zone(b)
							);

					ts = nl_add_timespec(a, b);
					nano = ts_to_nano(a) + ts_to_nano(b);
					failures += !!check_timetest(nano_to_ts(nano), ts, "addition", testname);
					tests++;

					ts = nl_sub_timespec(a, b);
					nano = ts_to_nano(a) - ts_to_nano(b);
					failures += !!check_timetest(nano_to_ts(nano), ts, "subtraction", testname);
					tests++;

					comp = nl_compare_timespec(a, b);
					nano = ts_to_nano(a) - ts_to_nano(b);
					nano = nano > 0 ? 1 : nano < 0 ? -1 : 0;
					failures += !!check_compare(nano, comp, "forward", testname);
					tests++;
				}
			}
		}
	}
}

static void explicit_operators(void)
{
	struct timespec ts;
	int comp;
	size_t i;

	INFO_OUT("Explicit tests of timespec operators\n");
	for(i = 0; i < ARRAY_SIZE(op_tests); i++) {
		struct timespec norm_a = nl_normalize_timespec(op_tests[i].a);
		struct timespec norm_b = nl_normalize_timespec(op_tests[i].b);

		DEBUG_OUT("Testing %s\n", op_tests[i].name);

		ts = nl_add_timespec(op_tests[i].a, op_tests[i].b);
		failures += !!check_timetest(op_tests[i].add, ts, "addition", op_tests[i].name);
		tests++;

		// Verify addition is commutative
		ts = nl_add_timespec(op_tests[i].b, op_tests[i].a);
		failures += !!check_timetest(op_tests[i].add, ts, "reversed addition", op_tests[i].name);
		tests++;

		// Verify prenormalization has no effect
		ts = nl_add_timespec(norm_a, norm_b);
		failures += !!check_timetest(op_tests[i].add, ts, "normalized addition", op_tests[i].name);
		tests++;

		ts = nl_sub_timespec(op_tests[i].a, op_tests[i].b);
		failures += !!check_timetest(op_tests[i].sub, ts, "subtraction", op_tests[i].name);
		tests++;

		ts = nl_sub_timespec(norm_a, norm_b);
		failures += !!check_timetest(op_tests[i].sub, ts, "normalized subtraction", op_tests[i].name);
		tests++;

		if(op_tests[i].neg_b.tv_sec || op_tests[i].neg_b.tv_nsec) {
			ts = nl_negate_timespec(op_tests[i].b);
			failures += !!check_timetest(op_tests[i].neg_b, ts, "negation", op_tests[i].name);
			tests++;
		}

		comp = nl_compare_timespec(norm_a, norm_b);
		failures += !!check_compare(op_tests[i].compare, comp, "forward", op_tests[i].name);
		tests++;

		// Verify comparison is symmetric
		comp = nl_compare_timespec(norm_b, norm_a);
		failures += !!check_compare(-op_tests[i].compare, comp, "reverse", op_tests[i].name);
		tests++;
	}

	if(failures) {
		ERROR_OUT("There were %d operator failures out of %d total tests\n", failures, tests);
	}
}

static void conversions(void)
{
	double v;

	// timeval
	v = nl_timeval_to_double((struct timeval){.tv_sec = 0, .tv_usec = 0});
	failures += !!check_convert_dbl(0, v, 1000000, "nl_timeval_to_double", "at time zero");

	v = nl_timeval_to_double((struct timeval){.tv_sec = 0, .tv_usec = 1});
	failures += !!check_convert_dbl(0.000001, v, 1000000, "nl_timeval_to_double", "at one microsecond past epoch");

	v = nl_timeval_to_double((struct timeval){.tv_sec = 1000000000, .tv_usec = 123456});
	failures += !!check_convert_dbl(1000000000.123456, v, 1000000, "nl_timeval_to_double", "precision at one billion");

	v = nl_timeval_to_double((struct timeval){.tv_sec = 4000000000, .tv_usec = 123456});
	failures += !!check_convert_dbl(4000000000.123456, v, 1000000, "nl_timeval_to_double", "precision at four billion");

	// timespec
	v = nl_timespec_to_double((struct timespec){.tv_sec = 0, .tv_nsec = 0});
	failures += !!check_convert_dbl(0, v, 1000000000, "nl_timespec_to_double", "at time zero");
	failures += !!check_timetest(
			(struct timespec){.tv_sec = 0, .tv_nsec = 0},
			nl_double_to_timespec(v),
			"nl_double_to_timespec",
			"at time zero"
			);

	v = nl_timespec_to_double((struct timespec){.tv_sec = 0, .tv_nsec = 1});
	failures += !!check_convert_dbl(0.000000001, v, 1000000000, "nl_timespec_to_double", "at one nanosecond past epoch");
	failures += !!check_timetest(
			(struct timespec){.tv_sec = 0, .tv_nsec = 1},
			nl_double_to_timespec(v),
			"nl_double_to_timespec",
			"at one nanosecond past epoch"
			);

	v = nl_timespec_to_double((struct timespec){.tv_sec = 1000000000, .tv_nsec = 123456789});
	failures += !!check_convert_dbl(1000000000.1234568, v, 1000000000, "nl_timespec_to_double", "precision at one billion");
	failures += !!check_timetest(
			(struct timespec){.tv_sec = 1000000000, .tv_nsec = 123456835}, // TODO: can this vary by platform?
			nl_double_to_timespec(v),
			"nl_double_to_timespec",
			"precision at one billion"
			);

	v = nl_timespec_to_double((struct timespec){.tv_sec = 4000000000, .tv_nsec = 123456789});
	failures += !!check_convert_dbl(4000000000.123457, v, 1000000000, "nl_timespec_to_double", "precision at four billion");
	failures += !!check_timetest(
			(struct timespec){.tv_sec = 4000000000, .tv_nsec = 123456954},
			nl_double_to_timespec(v),
			"nl_double_to_timespec",
			"precision at one billion"
			);
}

void operator_tests(void)
{
	INFO_OUT("Testing timespec arithmetic, comparison, and conversion operators\n");
	numeric_operators();
	explicit_operators();
	conversions();
}

void clock_tests(void)
{
	INFO_OUT("Testing functions for interacting with clocks and sleeping\n");

	const struct timespec interval = { .tv_sec = 1, .tv_nsec = 456789000 };
	struct timespec start = {0,0};
	struct timespec now = {0,0};
	struct timespec before = {0,0};
	struct timespec after = {0,0};

	nl_clock_fromnow(CLOCK_MONOTONIC, &start, (struct timespec){.tv_sec = 0});
	nl_clock_fromnow(CLOCK_MONOTONIC, &before, interval);
	nl_clock_fromnow(CLOCK_MONOTONIC, &after, nl_add_timespec(interval, (struct timespec){.tv_nsec = 500000}));
	nl_usleep(1678901); // Make sure nl_usleep can sleep longer than one second
	clock_gettime(CLOCK_MONOTONIC, &now);

	if(nl_compare_timespec(before, now) != -1) {
		ERROR_OUT("Expected before time was not before nl_usleep completion\n");
		failures += 1;
	}

	if(nl_compare_timespec(after, now) != -1) {
		ERROR_OUT("Expected after time was not after nl_usleep completion\n");
		failures += 1;
	}

	struct timespec diff = nl_sub_timespec(
			nl_sub_timespec(now, start),
			(struct timespec){.tv_sec = 1, .tv_nsec = 678901000}
			);
	if(diff.tv_sec || imaxabs(diff.tv_nsec) > 100000000) {
		ERROR_OUT("Sleep duration is off by more than 100ms\n");
		failures += 1;
	}
}

int main(void)
{
	operator_tests();
	clock_tests();

	if (failures) {
		ERROR_OUT("There were %d test failures.\n", failures);
		return -1;
	}

	INFO_OUT("All time tests succeeded.\n");

	return 0;
}
