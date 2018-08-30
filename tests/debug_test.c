/*
 * Tests debugging-related functions, such as backtrace generation.
 * Copyright (C)2018 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdio.h>
#include <dlfcn.h>
#include <signal.h>
#include <limits.h>
#include <execinfo.h>

#include "nlutils.h"

struct strsigcode_test {
	char *desc; // Test description
	int si_signo; // Signal number
	int si_code; // Signal-specific code
	char *expect; // String to expect within the strsigcode string
};

struct strsigcode_test strsigcode_tests[] = {
	{
		.desc = "Illegal opcode",
		.si_signo = SIGILL,
		.si_code = ILL_ILLOPC,
		.expect = "opcode",
	},
	{
		.desc = "Division by zero",
		.si_signo = SIGFPE,
		.si_code = FPE_INTDIV,
		.expect = "zero",
	},
	{
		.desc = "Generic signal with valid code (SIGINT/SI_TIMER)",
		.si_signo = SIGINT,
		.si_code = SI_TIMER,
		.expect = "expired",
	},
	{
		.desc = "Generic signal with invalid code (SIGINT/INT_MIN)",
		.si_signo = SIGINT,
		.si_code = INT_MIN,
		.expect = "unknown",
	},
	{
		.desc = "Specific signal with invalid code (SIGFPE/INT_MIN)",
		.si_signo = SIGFPE,
		.si_code = INT_MIN,
		.expect = "unknown",
	},
};

int test_nl_strsigcode(void)
{
	int ret = 0;

	for(size_t i = 0; i < ARRAY_SIZE(strsigcode_tests); i++) {
		struct strsigcode_test *t = &strsigcode_tests[i];
		const char *result = nl_strsigcode(t->si_signo, t->si_code);
		if(!strstr(result, t->expect)) {
			ERROR_OUT("Invalid result from nl_strsigcode() for signal %s code %d: expected %s within %s\n",
					strsignal(t->si_signo), t->si_code, t->expect, result);
			ret = -1;
		}
	}

	return ret;
}

int __attribute__((noinline)) nl_test_trace_level_four(FILE *f)
{
	NL_PRINT_TRACE(f);
	return 1;
}

int __attribute__((noinline)) nl_test_trace_level_three(FILE *f)
{
	return nl_test_trace_level_four(f) + 1;
}

int __attribute__((noinline)) nl_test_trace_level_two(FILE *f)
{
	return nl_test_trace_level_three(f) + 1;
}

int __attribute__((noinline)) nl_test_trace_level_one(FILE *f)
{
	return nl_test_trace_level_two(f) + 1;
}

int test_nl_print_backtrace(void)
{
	FILE *f = tmpfile();
	if(f == NULL) {
		ERRNO_OUT("Error creating temporary file for backtrace");
	}

	// Generate the backtrace
	nl_test_trace_level_one(f);

	// Rewind the file and check the backtrace
	rewind(f);
	struct nl_raw_data *trace = nl_read_stream(fileno(f));
	if(trace == NULL) {
		ERROR_OUT("Error reading backtrace from temporary file");
		fclose(f);
		return -1;
	}

	if(!strstr(trace->data, "level_one") || !strstr(trace->data, "level_two") ||
			!strstr(trace->data, "level_three") || !strstr(trace->data, "level_four")) {
		ERROR_OUT("Could not find expected function names in the generated backtrace:\n\n\e[31m%s\e[0m\n", trace->data);
		fclose(f);
		nl_destroy_data(trace);
		return -1;
	}

	fclose(f);
	nl_destroy_data(trace);
	return 0;
}

int main(void)
{
	int ret = 0;

	if(test_nl_strsigcode()) {
		ERROR_OUT("nl_strsigcode() tests failed.\n");
		ret = -1;
	} else {
		INFO_OUT("nl_strsigcode() tests succeeded.\n");
	}

	if(test_nl_print_backtrace()) {
		ERROR_OUT("nl_print_backtrace() tests failed.\n");
		ret = -1;
	} else {
		INFO_OUT("nl_print_backtrace() tests succeeded.\n");
	}

	return ret;
}
