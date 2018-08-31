/*
 * Tests debugging-related functions, such as backtrace generation.
 * Copyright (C)2018 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdio.h>
#include <dlfcn.h>
#include <signal.h>
#include <limits.h>
#include <execinfo.h>
#include <ucontext.h>

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

struct output_test {
	char *desc;
	int (*func)(FILE *); // Function to call with temporary file; nonzero return means test fails
	char **expect; // NULL-terminated list of strings to expect in the file
};

int run_output_test(struct output_test *test)
{
	FILE *f = tmpfile();
	if(f == NULL) {
		ERRNO_OUT("Error creating temporary file for %s", test->desc);
		return -1;
	}

	int ret = test->func(f);
	if(ret) {
		ERROR_OUT("Non-zero return %d from test function for %s\n", ret, test->desc);
		fclose(f);
		return -1;
	}

	rewind(f);
	struct nl_raw_data *result = nl_read_stream(fileno(f));
	if(result == NULL) {
		ERROR_OUT("Error reading %s results from temporary file", test->desc);
		fclose(f);
		return -1;
	}

	for(char **str = test->expect; *str; str++) {
		if(!strstr(result->data, *str)) {
			ERROR_OUT("Did not find expected string '%s' in %s output\n", *str, test->desc);
			ret = -1;
		}
	}

	if(ret) {
		ERROR_OUT("Actual output:\n\e[31m%s\e[0m\n", result->data);
	}

	nl_destroy_data(result);
	fclose(f);
	return ret;
}

// TODO: Something like Hedley looks interesting for the noinline attribute
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

int test_nl_print_backtrace(FILE *f)
{
	// Generate the backtrace
	nl_test_trace_level_one(f);
	return 0;
}

int test_nl_print_signal(FILE *f)
{
	nl_print_signal(f, "Testing 1 2 3", &(siginfo_t){.si_signo = SIGSEGV});
	return 0;
}

int test_nl_print_context(FILE *f)
{
	ucontext_t ctx;
	if(getcontext(&ctx)) {
		ERRNO_OUT("Error getting current execution context");
		return -1;
	}

	nl_print_context(f, &ctx);

	return 0;
}

struct output_test output_tests[] = {
	{
		.desc = "nl_print_backtrace()",
		.func = test_nl_print_backtrace,
		.expect = (char *[]){ "level_one", "level_two", "level_three", "level_four", NULL },
	},
	{
		.desc = "nl_print_signal()",
		.func = test_nl_print_signal,
		.expect = (char *[]){ "Testing 1 2 3", "Originating address", "11", "code 0", NULL },
	},
	{
		.desc = "nl_print_context()",
		.func = test_nl_print_context,
		.expect = (char *[]){ "Stack", "test_nl_print_context", "debug_test", NULL },
	},
};

int main(void)
{
	int ret = 0;

	if(test_nl_strsigcode()) {
		ERROR_OUT("nl_strsigcode() tests failed.\n");
		ret = -1;
	} else {
		INFO_OUT("nl_strsigcode() tests succeeded.\n");
	}

	for(size_t i = 0; i < ARRAY_SIZE(output_tests); i++) {
		struct output_test *t = &output_tests[i];
		if(run_output_test(t)) {
			ERROR_OUT("%s tests failed.\n", t->desc);
		} else {
			INFO_OUT("%s tests succeeded.\n", t->desc);
		}
	}

	return ret;
}
