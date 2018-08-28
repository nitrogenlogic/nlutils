/*
 * Tests debugging-related functions, such as backtrace generation.
 * Copyright (C)2018 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdio.h>
#include <dlfcn.h>
#include <signal.h>
#include <limits.h>

#include "nlutils.h"

struct strsigcode_test {
	char *desc; // Test description
	int si_signo; // Signal number
	int si_code; // Signal-specific code
	char *expect; // String to expect within the strsigcode string
};

static struct strsigcode_test strsigcode_tests[] = {
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

static int test_nl_strsigcode(void)
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

static int test_nl_find_symbol(void)
{
	puts("TODO: nl_find_symbol() tests, including tests for symbol table");
	return -1;
}

static int test_nl_print_backtrace(void)
{
	FILE *f = tmpfile();
	if(f == NULL) {
		ERRNO_OUT("Error creating temporary file for backtrace");
	}

	// XXX
	nl_print_backtrace(stdout, (void *[]){(void *)test_nl_print_backtrace, (void *)test_nl_find_symbol, (void *)strsigcode_tests}, 3);

	fclose(f);

	puts("TODO: backtrace tests");
	return -1;
}


int main(void)
{
	int ret = 0;

	if(nl_load_symbols() < 0) {
		ERROR_OUT("Error loading symbol table with nl_load_symbols().\n");
		return -1;
	}

	if(test_nl_strsigcode()) {
		ERROR_OUT("nl_strsigcode() tests failed.\n");
		ret = -1;
	}

	if(test_nl_find_symbol()) {
		ERROR_OUT("nl_find_symbol() tests failed.\n");
		ret = -1;
	}

	if(test_nl_print_backtrace()) {
		ERROR_OUT("nl_print_backtrace() tests failed.\n");
		ret = -1;
	}

	nl_unload_symbols();

	return ret;
}
