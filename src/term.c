/*
 * Terminal-related utility functions, such as ANSI color escape parsing.
 *
 * Written in part as an exercise.  There are probably off-the-shelf tools that
 * one should use instead.
 *
 * Copyright (C)2023 Mike Bourgeous.  Licensed under AGPLv3.
 */

#include <stdlib.h>
#include <ctype.h>

#include "nlutils.h"

/*
 * The default foreground color if no explicit color is set and neither bold
 * nor faint intensity is active.
 */
const struct nl_term_color nl_term_default_foreground = NL_TERM_FOREGROUND_INITIALIZER;

#define NL_TERM_BOLD_INITIALIZER { \
	.r = 234, .g = 234, .b = 234, \
	.xterm256 = 15, \
	.ansi = 7, \
	.color_type = NL_TERM_COLOR_DEFAULT, \
}

#define NL_TERM_FAINT_INITIALIZER { \
	.r = 80, .g = 80, .b = 80, \
	.xterm256 = 7, \
	.ansi = 7, \
	.color_type = NL_TERM_COLOR_DEFAULT, \
}

/*
 * The default bold foreground color if no explicit color is set but color
 * intensity is set to bold.
 */
const struct nl_term_color nl_term_bold_foreground = NL_TERM_BOLD_INITIALIZER;

/*
 * The default faint foreground color if no explicit color is set, but color
 * intensity is set to faint.
 */
const struct nl_term_color nl_term_faint_foreground = NL_TERM_FAINT_INITIALIZER;

/*
 * The default background color if no explicit color is set.
 */
const struct nl_term_color nl_term_default_background = NL_TERM_BACKGROUND_INITIALIZER;

/*
 * Standard terminal colors in normal, bold, and faint intensities (0 through
 * 7), plus default colors for foreground (8) and background (9).
 */
const struct nl_term_color nl_term_standard_colors[3][10] = {
	[ NL_TERM_NORMAL ] = {
		[0] = {  36,  36,  36, 0, 0, NL_TERM_COLOR_STANDARD },
		[1] = { 204,  66,  66, 1, 1, NL_TERM_COLOR_STANDARD },
		[2] = { 104, 154,  51, 2, 2, NL_TERM_COLOR_STANDARD },
		[3] = { 196, 165,  42, 3, 3, NL_TERM_COLOR_STANDARD },
		[4] = {  61, 107, 164, 4, 4, NL_TERM_COLOR_STANDARD },
		[5] = { 116,  80, 123, 5, 5, NL_TERM_COLOR_STANDARD },
		[6] = {  63, 154, 154, 6, 6, NL_TERM_COLOR_STANDARD },
		[7] = { 158, 158, 158, 7, 7, NL_TERM_COLOR_STANDARD },

		// Default colors (foreground, background)
		[8] = NL_TERM_FOREGROUND_INITIALIZER,
		[9] = NL_TERM_BACKGROUND_INITIALIZER,
	},
	[ NL_TERM_BOLD ] = {
		[0] = {  70,  70,  70,  8, 0, NL_TERM_COLOR_STANDARD },
		[1] = { 225,  70,  70,  9, 1, NL_TERM_COLOR_STANDARD },
		[2] = { 138, 196,  81, 10, 2, NL_TERM_COLOR_STANDARD },
		[3] = { 205, 189,  83, 11, 3, NL_TERM_COLOR_STANDARD },
		[4] = {  85, 146, 207, 12, 4, NL_TERM_COLOR_STANDARD },
		[5] = { 173, 115, 167, 13, 5, NL_TERM_COLOR_STANDARD },
		[6] = {  78, 190, 190, 14, 6, NL_TERM_COLOR_STANDARD },
		[7] = { 234, 234, 234, 15, 7, NL_TERM_COLOR_STANDARD },

		[8] = NL_TERM_BOLD_INITIALIZER,
		[9] = NL_TERM_BACKGROUND_INITIALIZER,
	},
	[ NL_TERM_FAINT ] = {
		[0] = {  24,  24,  24, 0, 0, NL_TERM_COLOR_STANDARD },
		[1] = { 140,  41,  41, 1, 1, NL_TERM_COLOR_STANDARD },
		[2] = {  70, 100,  41, 2, 2, NL_TERM_COLOR_STANDARD },
		[3] = { 100,  87,  40, 3, 3, NL_TERM_COLOR_STANDARD },
		[4] = {  39,  78, 125, 4, 4, NL_TERM_COLOR_STANDARD },
		[5] = {  95,  65, 100, 5, 5, NL_TERM_COLOR_STANDARD },
		[6] = {  35,  80,  80, 6, 6, NL_TERM_COLOR_STANDARD },
		[7] = {  80,  80,  80, 7, 7, NL_TERM_COLOR_STANDARD },

		[8] = NL_TERM_BOLD_INITIALIZER,
		[9] = NL_TERM_BACKGROUND_INITIALIZER,
	},
};

// The 32 gray levels from xterm's 256-color palette starting at index 232.
// Reference: https://en.wikipedia.org/wiki/ANSI_escape_code#8-bit
const struct nl_term_color nl_term_xterm_grays[32] = {
	{ 0x08, 0x08, 0x08, 232, 0, NL_TERM_COLOR_XTERM256 },
	{ 0x12, 0x12, 0x12, 233, 0, NL_TERM_COLOR_XTERM256 },
	{ 0x1c, 0x1c, 0x1c, 234, 0, NL_TERM_COLOR_XTERM256 },
	{ 0x26, 0x26, 0x26, 235, 0, NL_TERM_COLOR_XTERM256 },
	{ 0x30, 0x30, 0x30, 236, 0, NL_TERM_COLOR_XTERM256 },
	{ 0x3a, 0x3a, 0x3a, 237, 0, NL_TERM_COLOR_XTERM256 },
	{ 0x44, 0x44, 0x44, 238, 0, NL_TERM_COLOR_XTERM256 },
	{ 0x4e, 0x4e, 0x4e, 239, 0, NL_TERM_COLOR_XTERM256 },
	{ 0x58, 0x58, 0x58, 240, 0, NL_TERM_COLOR_XTERM256 },
	{ 0x62, 0x62, 0x62, 241, 0, NL_TERM_COLOR_XTERM256 },
	{ 0x6c, 0x6c, 0x6c, 242, 0, NL_TERM_COLOR_XTERM256 },
	{ 0x76, 0x76, 0x76, 243, 0, NL_TERM_COLOR_XTERM256 },
	{ 0x80, 0x80, 0x80, 244, 0, NL_TERM_COLOR_XTERM256 },
	{ 0x8a, 0x8a, 0x8a, 245, 0, NL_TERM_COLOR_XTERM256 },
	{ 0x94, 0x94, 0x94, 246, 0, NL_TERM_COLOR_XTERM256 },
	{ 0x9e, 0x9e, 0x9e, 247, 0, NL_TERM_COLOR_XTERM256 },
	{ 0xa8, 0xa8, 0xa8, 248, 0, NL_TERM_COLOR_XTERM256 },
	{ 0xb2, 0xb2, 0xb2, 249, 0, NL_TERM_COLOR_XTERM256 },
	{ 0xbc, 0xbc, 0xbc, 250, 0, NL_TERM_COLOR_XTERM256 },
	{ 0xc6, 0xc6, 0xc6, 251, 0, NL_TERM_COLOR_XTERM256 },
	{ 0xd0, 0xd0, 0xd0, 252, 0, NL_TERM_COLOR_XTERM256 },
	{ 0xda, 0xda, 0xda, 253, 0, NL_TERM_COLOR_XTERM256 },
	{ 0xe4, 0xe4, 0xe4, 254, 0, NL_TERM_COLOR_XTERM256 },
	{ 0xee, 0xee, 0xee, 255, 0, NL_TERM_COLOR_XTERM256 },
};

// Xterm jumps from 0 to 95, then uses increments of 40, for the 6 values
// for R, G, or B in its 216-color cube for 256-color mode.
static const uint8_t xterm_rgb_values[6] = {
	0,
	95,
	135,
	175,
	215,
	255
};

/* Default terminal state, for use as an initializer. */
const struct nl_term_state nl_default_term_state = NL_TERM_STATE_INITIALIZER;

/*
 * Sets default state values on the given terminal state struct.
 */
void nl_init_term_state(struct nl_term_state *s)
{
	*s = nl_default_term_state;
}

static void make_color_faint(struct nl_term_color *c)
{
	switch(c->color_type) {
		case NL_TERM_COLOR_DEFAULT:
			*c = nl_term_faint_foreground;
			break;

		case NL_TERM_COLOR_STANDARD:
			*c = nl_term_standard_colors[NL_TERM_FAINT][c->ansi];
			break;

		case NL_TERM_COLOR_XTERM256:
			// Xterm-256 colors 0..15 are standard and bold colors
			if (c->xterm256 < 8) {
				*c = nl_term_standard_colors[NL_TERM_FAINT][c->xterm256];
			} else if (c->xterm256 < 16) {
				*c = nl_term_standard_colors[NL_TERM_FAINT][c->xterm256 - 8];
			}
			break;

		default:
			// Do nothing for RGB colors
			break;
	}
}

static void make_color_intense(struct nl_term_color *c)
{
	switch(c->color_type) {
		case NL_TERM_COLOR_DEFAULT:
			*c = nl_term_bold_foreground;
			break;

		case NL_TERM_COLOR_STANDARD:
			*c = nl_term_standard_colors[NL_TERM_INTENSE][c->ansi];
			break;

		case NL_TERM_COLOR_XTERM256:
			if (c->xterm256 < 8) {
				*c = nl_term_standard_colors[NL_TERM_INTENSE][c->xterm256];
			}
			break;

		default:
			// Do nothing for RGB colors
			break;
	}
}

static void make_color_normal(struct nl_term_color *c)
{
	switch(c->color_type) {
		case NL_TERM_COLOR_DEFAULT:
			*c = nl_term_default_foreground;
			break;

		case NL_TERM_COLOR_STANDARD:
			*c = nl_term_standard_colors[NL_TERM_NORMAL][c->ansi];
			break;

		case NL_TERM_COLOR_XTERM256:
			if (c->xterm256 >= 8 && c->xterm256 < 16) {
				*c = nl_term_standard_colors[NL_TERM_NORMAL][c->xterm256 - 8];
			}
			break;

		default:
			// Do nothing for RGB colors
			break;
	}
}

// Used by the parser to update the foreground color when the bold/intensity setting changes.
static void set_color_intensity(struct nl_term_state *s)
{
	switch(s->intensity) {
		case NL_TERM_NORMAL:
			make_color_normal(&s->fg);
			break;

		case NL_TERM_INTENSE:
			make_color_intense(&s->fg);
			break;

		case NL_TERM_FAINT:
			make_color_faint(&s->fg);
			break;
	}
}

// Used in the parser to control whether an 8-bit or 24-bit color is written to the foreground or background.
enum target_color {
	FOREGROUND_COLOR,
	BACKGROUND_COLOR
};

// Used in the parser to keep track of what type of value is expected next.
enum color_parse_state {
	EXPECT_CONTROL,
	EXPECT_SEMICOLON,
	EXPECT_2_OR_5,
	EXPECT_XTERM_256,
	EXPECT_RED,
	EXPECT_GREEN,
	EXPECT_BLUE,
};

// Used by the parser to write an 8-bit or 24-bit color to the right field in the terminal state.
static void set_target_color(struct nl_term_state *s, enum target_color color_target, struct nl_term_color *c)
{
	if (color_target == BACKGROUND_COLOR) {
		s->bg = *c;
	} else {
		s->fg = *c;
	}
}

#define MAX_COLOR_PARSE_LENGTH (128 * 4) // More than enough room to include every code once
#define MAX_COLOR_PARSE_LOOPS (128 * 2) // Enough to include every code once (times two for semicolons)

/*
 * Parses an ANSI color sequence at the start of the given string.  Returns the
 * number of characters consumed, 0 if the sequence could not be parsed as an
 * ANSI color (thus no characters consumed), or -1 on error.
 *
 * The given terminal state will be updated with the changes described by the
 * escape sequence in s.  The state is only updated if a full, valid escape
 * sequence can be parsed.
 *
 * Example valid string prefixes (anything after the m is ignored):
 *     "\e[1m" -- turns on bold, returns 4
 *     "\e[35m" -- sets a purple/magenta foreground, returns 5
 *     "\e[38;2;128;128;128m" -- sets a dark gray 24-bit foreground, returns 19
 */
int nl_term_parse_ansi_color(char *s, struct nl_term_state *state)
{
	if (CHECK_NULL(s) || CHECK_NULL(state)) {
		return -1;
	}

	struct nl_term_state new_state = *state;

	if (s[0] != 0x1b || s[1] != '[') {
		// Not the beginning of an ANSI escape sequence.
		DEBUG_OUT("Missing escape and left-bracket at start of ANSI color sequence.\n");
		return 0;
	}

	// TODO: konsole ignores multiple sequential semicolons unless they're within a 256-color or 24-bit-color sequence; maybe we should do the same
	// TODO: maybe implement a maximum length to guard against extra-long adversarial inputs?

	char *ptr = s + 2;
	char *endptr = NULL;
	enum color_parse_state ps = EXPECT_CONTROL;
	enum color_parse_state next_ps = EXPECT_SEMICOLON;
	enum target_color color_target = FOREGROUND_COLOR;
	struct nl_term_color parse_color = nl_term_default_foreground;
	unsigned long n;
	unsigned int loops = 0;
	while(*ptr) {
		DEBUG_OUT("Color parsing offset %td ('%.4s...'), current state %d, next state %d\n", ptr - s, ptr, ps, next_ps);

		if (ptr - s > MAX_COLOR_PARSE_LENGTH) {
			DEBUG_OUT("Maximum ANSI color sequence length of %d exceeded\n", MAX_COLOR_PARSE_LENGTH);
			return 0;
		}

		if (loops > MAX_COLOR_PARSE_LOOPS) {
			ERROR_OUT("Maximum color sequence parsing loop count of %d exceeded; this may be a bug\n", MAX_COLOR_PARSE_LOOPS);
			return 0;
		}

		if (*ptr == 'm') {
			// End of sequence; update state and return number of characters in sequence
			DEBUG_OUT("Found end of ANSI color sequence.  Updating state.\n");
			*state = new_state;
			return ptr + 1 - s;
		}

		switch(ps) {
			case EXPECT_SEMICOLON:
				DEBUG_OUT("Looking for a semicolon.\n");

				if (*ptr != ';') {
					// This should have been a semicolon
					DEBUG_OUT("Expected a semicolon, saw '%c'.\n", *ptr);
					return 0;
				}
				ptr++;

				ps = next_ps;
				next_ps = EXPECT_CONTROL;
				break;

			case EXPECT_CONTROL:
				DEBUG_OUT("Looking for a control code.\n");

				next_ps = EXPECT_CONTROL;

				n = strtoul(ptr, &endptr, 10);
				if (endptr == NULL || endptr == ptr) {
					DEBUG_OUT("Did not find a number when looking for a control code.\n");
					return 0;
				}
				ptr = endptr;

				// I bet that some parsers and the original hardware just use the ones digit as a bit index to set or clear for 1..9 and 21..29
				switch(n) {
					case 0:
						DEBUG_OUT("Code 0, reset state\n");
						new_state = nl_default_term_state;
						break;

					case 1:
						DEBUG_OUT("Code 1, turn on bold\n");
						new_state.intensity = NL_TERM_INTENSE;
						set_color_intensity(&new_state);
						break;

					case 2:
						DEBUG_OUT("Code 2, turn on faint\n");
						new_state.intensity = NL_TERM_FAINT;
						set_color_intensity(&new_state);
						break;

					case 3:
						DEBUG_OUT("Code 3, turn on italic\n");
						new_state.italic = 1;
						break;

					case 4:
						DEBUG_OUT("Code 4, turn on underline\n");
						new_state.underline = 1;
						break;

					case 5:
						DEBUG_OUT("Code 5, turn on blink\n");
						new_state.blink = 1;
						break;

					case 7:
						DEBUG_OUT("Code 7, turn on reverse\n");
						new_state.reverse = 1;
						break;

					case 9:
						DEBUG_OUT("Code 9, turn on strikethrough\n");
						new_state.strikethrough = 1;
						break;

					case 22:
						DEBUG_OUT("Code 22, turn off bold or faint\n");
						new_state.intensity = NL_TERM_NORMAL;
						set_color_intensity(&new_state);
						break;

					case 23:
						DEBUG_OUT("Code 23, turn off italic\n");
						new_state.italic = 0;
						break;

					case 24:
						DEBUG_OUT("Code 24, turn off underline\n");
						new_state.underline = 0;
						break;

					case 25:
						DEBUG_OUT("Code 25, turn off blink\n");
						new_state.blink = 0;
						break;

					case 27:
						DEBUG_OUT("Code 27, turn off reverse\n");
						new_state.reverse = 0;
						break;

					case 29:
						DEBUG_OUT("Code 29, turn off strikethrough\n");
						new_state.strikethrough = 0;
						break;

					case 38:
						DEBUG_OUT("Code 38, special foreground color\n");
						ps = EXPECT_SEMICOLON;
						next_ps = EXPECT_2_OR_5;
						color_target = FOREGROUND_COLOR;
						break;

					case 39:
						DEBUG_OUT("Code 39, reset foreground color\n");
						new_state.fg = nl_term_default_foreground;
						set_color_intensity(&new_state);
						break;

					case 48:
						DEBUG_OUT("Code 48, special background color\n");
						ps = EXPECT_SEMICOLON;
						next_ps = EXPECT_2_OR_5;
						color_target = BACKGROUND_COLOR;
						break;

					case 49:
						DEBUG_OUT("Code 49, reset background color\n");
						new_state.bg = nl_term_default_background;
						break;

					default:
						// Handle standard colors, ignore unsupported values
						if (n >= 30 && n <= 37) {
							DEBUG_OUT("Code %lu, set foreground color %lu\n", n, n - 30);
							new_state.fg = nl_term_standard_colors[new_state.intensity][n - 30];
						} else if (n >= 40 && n <= 47) {
							DEBUG_OUT("Code %lu, set background color %lu\n", n, n - 40);
							new_state.bg = nl_term_standard_colors[NL_TERM_NORMAL][n - 40];
						} else {
							DEBUG_OUT("Unknown code %lu, ignoring.\n", n);
						}

						break;
				}

				ps = EXPECT_SEMICOLON;
				break;

			case EXPECT_2_OR_5:
				DEBUG_OUT("Looking for 2 for RGB, or 5 for xterm-256.\n");

				// If we see 2, expect a 24-bit color.  If we see 5, expect an xterm color.
				// For other values, xterm ignores them, while konsole resets the color and treats the
				// value as a standard control code.  Let's do what xterm does.
				n = strtoul(ptr, &endptr, 10);
				if (endptr == NULL || endptr == ptr) {
					DEBUG_OUT("Did not find a number when looking for 2 (RGB) or 5 (8-bit) after 38 (foreground) or 48 (background).\n");
					return 0;
				}
				ptr = endptr;

				switch(n) {
					case 2:
						// RGB color
						DEBUG_OUT("Code 2, RGB.\n");
						next_ps = EXPECT_RED;
						parse_color.xterm256 = 0;
						parse_color.ansi = 0;
						parse_color.color_type = NL_TERM_COLOR_RGB;
						break;

					case 5:
						// Xterm 256 color
						DEBUG_OUT("Code 5, xterm-256.\n");
						next_ps = EXPECT_XTERM_256;
						parse_color.ansi = 0;
						parse_color.color_type = NL_TERM_COLOR_XTERM256;
						break;

					default:
						// Ignore anything else like xterm seems to do
						DEBUG_OUT("Ignoring unexpected color type %lu\n", n);
						next_ps = EXPECT_CONTROL;
						break;
				}

				ps = EXPECT_SEMICOLON;
				break;

			case EXPECT_XTERM_256:
				DEBUG_OUT("Looking for an xterm-256color color index\n");

				n = strtoul(ptr, &endptr, 10);
				if (endptr == NULL || endptr == ptr) {
					DEBUG_OUT("Did not find a number when looking for an 8-bit xterm 256-color color index.\n");
					return 0;
				}
				ptr = endptr;

				n %= 256;

				if (n < 8) {
					parse_color = nl_term_standard_colors[NL_TERM_NORMAL][n];
				} else if (n < 16) {
					parse_color = nl_term_standard_colors[NL_TERM_INTENSE][n - 8];
				} else if (n < 232) {
					uint8_t rgb = n - 16;
					parse_color = (struct nl_term_color){
						.r = xterm_rgb_values[rgb / 36],
						.g = xterm_rgb_values[(rgb / 6) % 6],
						.b = xterm_rgb_values[rgb % 6],

						.xterm256 = n,

						.color_type = NL_TERM_COLOR_XTERM256,
					};
				} else {
					parse_color = nl_term_xterm_grays[n - 232];
				}

				parse_color.color_type = NL_TERM_COLOR_XTERM256;

				set_target_color(&new_state, color_target, &parse_color);

				ps = EXPECT_SEMICOLON;
				next_ps = EXPECT_CONTROL;
				break;

			case EXPECT_RED:
			case EXPECT_GREEN:
			case EXPECT_BLUE:
				DEBUG_OUT("Looking for an RGB color component: %s\n",
						ps == EXPECT_RED ? "red" :
						ps == EXPECT_GREEN ? "green" :
						ps == EXPECT_BLUE ? "blue" :
						"BUG"
						);

				n = strtoul(ptr, &endptr, 10);
				if (endptr == NULL || endptr == ptr) {
					DEBUG_OUT("Did not find a number when looking for an 8-bit RGB color component.\n");
					return 0;
				}
				ptr = endptr;

				n %= 256;

				switch (ps) {
					default:
					case EXPECT_RED:
						parse_color.r = n;
						next_ps = EXPECT_GREEN;
						break;

					case EXPECT_GREEN:
						parse_color.g = n;
						next_ps = EXPECT_BLUE;
						break;

					case EXPECT_BLUE:
						parse_color.b = n;
						set_target_color(&new_state, color_target, &parse_color);
						next_ps = EXPECT_CONTROL;
						break;
				}

				ps = EXPECT_SEMICOLON;
				break;
		}
	}

	// No 'm' was found before the end of the string, so don't consume any characters.
	DEBUG_OUT("End of string was reached before finding terminating 'm' character.\n");
	return 0;
}
