/*
 * Tests for terminal-related functions.
 * Copyright (C)2023 Mike Bourgeous.  Licensed under AGPLv3.
 */

#include <stdio.h>
#include <stdlib.h>

#include "nlutils.h"

struct parse_color_test {
	char *desc;
	char *input;
	int expected_return;
	struct nl_term_state init_state;
	struct nl_term_state expected_state;
};

#define ALL_ON_INITIALIZER { \
	.fg = { 253, 254, 255, 0, 0, NL_TERM_COLOR_RGB }, \
	.bg = { 5, 4, 3, 0, 0, NL_TERM_COLOR_RGB }, \
	.intensity = NL_TERM_INTENSE, \
	.italic = 1, \
	.underline = 1, \
	.blink = 1, \
	.reverse = 1, \
	.strikethrough = 1, \
}

#define XTERM_256_LOW_INITIALIZER { \
	.fg = { 63, 154, 154, 6, 6, NL_TERM_COLOR_XTERM256 }, \
	.bg = { 205, 189, 83, 11, 3, NL_TERM_COLOR_XTERM256 }, \
}

#define XTERM_256_RGB_INITIALIZER { \
	.fg = { 0xaf, 0x87, 0xd7, 140, 0, NL_TERM_COLOR_XTERM256 }, \
	.bg = { 0x87, 0x5f, 0xd7, 98, 0, NL_TERM_COLOR_XTERM256 }, \
}

#define XTERM_256_GRAY_INITIALIZER {\
	.fg = { 8, 8, 8, 232, 0, NL_TERM_COLOR_XTERM256 }, \
	.bg = { 0xbc, 0xbc, 0xbc, 250, 0, NL_TERM_COLOR_XTERM256 }, \
}

static struct parse_color_test color_tests[] = {
	{
		.desc = "Empty valid sequence, no change to state (from zero state)",
		.input = "\e[m",
		.expected_return = 3,
	},
	{
		.desc = "Empty valid sequence, no change to state (from default state)",
		.input = "\e[m",
		.expected_return = 3,
		.init_state = NL_TERM_STATE_INITIALIZER,
		.expected_state = NL_TERM_STATE_INITIALIZER,
	},
	{
		.desc = "Empty valid sequence, no change to state (from modified state)",
		.input = "\e[m",
		.expected_return = 3,
		.init_state = ALL_ON_INITIALIZER,
		.expected_state = ALL_ON_INITIALIZER,
	},
	{
		.desc = "Invalid sequence does not alter state",
		.input = "\e[1;3;4;5;7;9;38;2;253;254;255;48;2;5;4;3K",
		.expected_return = 0,
		.init_state = ALL_ON_INITIALIZER,
		.expected_state = ALL_ON_INITIALIZER,
	},
	{
		.desc = "Reset to default",
		.input = "\e[0m",
		.expected_return = 4,
		.init_state = ALL_ON_INITIALIZER,
		.expected_state = NL_TERM_STATE_INITIALIZER,
	},
	{
		.desc = "Reset to default, ignoring text after the escape sequence",
		.input = "\e[0m\e[1m\e[33m This is all ignored",
		.expected_return = 4,
		.init_state = ALL_ON_INITIALIZER,
		.expected_state = NL_TERM_STATE_INITIALIZER,
	},
	{
		.desc = "Generate all-on state (RGB colors, turn on all flags)",
		.input = "\e[1;3;4;5;7;9;38;2;253;254;255;48;2;5;4;3m",
		.expected_return = 42,
		.init_state = NL_TERM_STATE_INITIALIZER,
		.expected_state = ALL_ON_INITIALIZER,
	},
	{
		.desc = "Generate default state from all-on state",
		.input = "\e[22;23;24;25;27;29;39;49m",
		.expected_return = 26,
		.init_state = ALL_ON_INITIALIZER,
		.expected_state = NL_TERM_STATE_INITIALIZER,
	},
	{
		.desc = "Parse xterm-256 colors in range 0..15 (ANSI)",
		.input = "\e[38;5;6;48;5;11m",
		.expected_return = 17,
		.init_state = NL_TERM_STATE_INITIALIZER,
		.expected_state = XTERM_256_LOW_INITIALIZER,
	},
	{
		.desc = "Parse xterm-256 colors in range 16..231 (RGB)",
		.input = "\e[38;5;140;48;5;98m",
		.expected_return = 19,
		.init_state = NL_TERM_STATE_INITIALIZER,
		.expected_state = XTERM_256_RGB_INITIALIZER,
	},
	{
		.desc = "Parse xterm-256 colors in range 232..255 (gray)",
		.input = "\e[38;5;232;48;5;250m",
		.expected_return = 20,
		.init_state = NL_TERM_STATE_INITIALIZER,
		.expected_state = XTERM_256_GRAY_INITIALIZER,
	},
};

#define COMPARE_FIELD(ret, msg, format, field, actual, expected) do { \
	if ((actual)->field != (expected)->field) { \
		ERROR_OUT("\t\t%s %s->%s = " format " but %s->%s = " format "\n", \
				(msg), #actual, #field, (actual)->field, #expected, #field, (expected)->field); \
		ret++; \
	} \
} while (0);

static int compare_term_color(char *msg, struct nl_term_color *c, struct nl_term_color *expected)
{
	DEBUG_OUT("\e[36mSize of color is \e[1m%zu\e[0m\n", sizeof(struct nl_term_color));

	int ret = 0;

	// Wish there was a way to iterate over a struct in C
	COMPARE_FIELD(ret, msg, "%u", r, c, expected);
	COMPARE_FIELD(ret, msg, "%u", g, c, expected);
	COMPARE_FIELD(ret, msg, "%u", b, c, expected);
	COMPARE_FIELD(ret, msg, "%u", xterm256, c, expected);
	COMPARE_FIELD(ret, msg, "%u", ansi, c, expected);
	COMPARE_FIELD(ret, msg, "%d", color_type, c, expected);

	return ret;
}

static int compare_term_state(char *test_name, struct nl_term_state *s, struct nl_term_state *expected)
{
	DEBUG_OUT("\e[36mSize of state is \e[1m%zu\e[0m\n", sizeof(struct nl_term_state));

	if (s == NULL && expected != NULL) {
		ERROR_OUT("Terminal state is NULL but expected is not NULL for %s.\n", test_name);
		return -1;
	}
	if (s != NULL && expected == NULL) {
		ERROR_OUT("Terminal state is not NULL but expected is NULL for %s.\n", test_name);
		return 1;
	}
	if (s == expected) {
		if (s != NULL) {
			ERROR_OUT("Do not pass the same non-NULL pointer for received state and expected state (test %s).\n", test_name);
			return -1;
		}

		DEBUG_OUT("Both received and expected state pointers are NULL for %s.\n", test_name);
		return 0;
	}

	int ret = 0;

	if (compare_term_color("\tforeground", &s->fg, &expected->fg)) {
		ERROR_OUT("Foreground color did not match for test %s\n", test_name);
		ret++;
	}
	if (compare_term_color("\tbackground", &s->bg, &expected->bg)) {
		ERROR_OUT("Background color did not match for test %s\n", test_name);
		ret++;
	}

	COMPARE_FIELD(ret, test_name, "%d", intensity, s, expected);
	COMPARE_FIELD(ret, test_name, "%u", italic, s, expected);
	COMPARE_FIELD(ret, test_name, "%u", underline, s, expected);
	COMPARE_FIELD(ret, test_name, "%u", blink, s, expected);
	COMPARE_FIELD(ret, test_name, "%u", reverse, s, expected);
	COMPARE_FIELD(ret, test_name, "%u", strikethrough, s, expected);

	return ret;
}

int test_nl_term_parse_ansi_color(void)
{
	struct nl_term_state state = NL_TERM_STATE_INITIALIZER;
	int failures = 0;
	int result;

	INFO_OUT("Testing nl_term_parse_ansi_color()\n");

	result = nl_term_parse_ansi_color(NULL, NULL);
	if (result != -1) {
		ERROR_OUT("Expected -1 for null input and null state\n");
		failures += 1;
	}

	result = nl_term_parse_ansi_color(NULL, &state);
	if (result != -1) {
		ERROR_OUT("Expected -1 for null input and valid state\n");
		failures += 1;
	}

	result = nl_term_parse_ansi_color("\e[0m", NULL);
	if (result != -1) {
		ERROR_OUT("Expected -1 for valid input and null state\n");
		failures += 1;
	}

	for (size_t i = 0; i < ARRAY_SIZE(color_tests); i++) {
		struct parse_color_test *t = &color_tests[i];

		DEBUG_OUT("\tColor parsing test %zu: %s\n", i, t->desc);
		state = t->init_state;

		result = nl_term_parse_ansi_color(t->input, &state);
		if (result != t->expected_return) {
			ERROR_OUT("\tExpected return value of %d, got %d for test %s\n", t->expected_return, result, t->desc);
			failures += 1;
		}

		if (compare_term_state(t->desc, &state, &t->expected_state)) {
			failures += 1;
		}
	}

	return failures;
}

int main(void)
{
	int failures = 0;

	INFO_OUT("Testing terminal functions\n");

	failures += test_nl_term_parse_ansi_color();

	if (failures) {
		ERROR_OUT("%d terminal function tests failed\n", failures);
	} else {
		INFO_OUT("All terminal function tests succeeded\n");
	}

	return !!failures;
}
