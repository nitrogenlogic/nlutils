/*
 * Terminal-related utility functions, such as ANSI color escape parsing.
 *
 * Written in part as an exercise.  There are probably off-the-shelf tools that
 * one should use instead.
 *
 * Copyright (C)2023 Mike Bourgeous, licensed under AGPLv3.
 */

#include <ctype.h>

#include "nlutils.h"

/*
 * The default foreground color if no explicit color is set.
 */
const struct nl_term_color nl_term_default_foreground = {
	.r = 158, .g = 158, .b = 158,
	.color_type = NL_TERM_COLOR_DEFAULT,
};

/*
 * The default bold foreground color if no explicit color is set but color
 * intensity is set to bold.
 */
const struct nl_term_color nl_term_bold_foreground = {
	.r = 234, .g = 234, .b = 234,
	.color_type = NL_TERM_COLOR_DEFAULT,
};

/*
 * The default bold foreground color if no explicit color is set but color
 * intensity is set to faint.
 */
const struct nl_term_color nl_term_faint_foreground = {
	.r = 234, .g = 234, .b = 234,
	.color_type = NL_TERM_COLOR_DEFAULT,
};

/*
 * The default background color if no explicit color is set.
 */
const struct nl_term_color nl_term_default_background = {
	.r = 16, .g = 16, .b = 16,
	.color_type = NL_TERM_COLOR_DEFAULT,
};

/*
 * Standard terminal colors in normal, bold, and faint intensities (0 through
 * 7), plus default colors for foreground (8) and background (9).
 */
const struct nl_term_color nl_term_standard_colors[3][10] = {
	[ NL_TERM_NORMAL ] = {
		[0] = {  36,  36,  36, 0, 0 },
		[1] = { 204,  66,  66, 1, 1 },
		[2] = { 104, 154,  51, 2, 2 },
		[3] = { 196, 165,  42, 3, 3 },
		[4] = {  61, 107, 164, 4, 4 },
		[5] = { 116,  80, 123, 5, 5 },
		[6] = {  63, 154, 154, 6, 6 },
		[7] = { 158, 158, 158, 7, 7 },

		// Default colors (foreground, background)
		[8] = nl_term_default_foreground,
		[9] = nl_term_default_background,
	},
	[ NL_TERM_BOLD ] = {
		[0] = {  70,  70,  70,  8, 0 },
		[1] = { 225,  70,  70,  9, 1 },
		[2] = { 138, 196,  81, 10, 2 },
		[3] = { 205, 189,  83, 11, 3 },
		[4] = {  85, 146, 207, 12, 4 },
		[5] = { 173, 115, 167, 13, 5 },
		[6] = {  78, 190, 190, 14, 6 },
		[7] = { 234, 234, 234, 15, 7 },

		[8] = nl_term_bold_foreground,
		[9] = nl_term_default_background,
	},
	[ NL_TERM_FAINT ] = {
		[0] = {  24,  24,  24, 0, 0 },
		[1] = { 140,  41,  41, 1, 1 },
		[2] = {  70, 100,  41, 2, 2 },
		[3] = { 100,  87,  40, 3, 3 },
		[4] = {  39,  78, 125, 4, 4 },
		[5] = {  95,  65, 100, 5, 5 },
		[6] = {  35,  80,  80, 6, 6 },
		[7] = {  80,  80,  80, 7, 7 },

		[8] = nl_term_bold_foreground,
		[9] = nl_term_default_background,
	},
};

/* Default terminal state, for use as an initializer. */
const struct nl_term_state nl_default_term_state = {
	.fg = nl_term_default_foreground,
	.bg = nl_term_default_background,

	.intensity = NL_TERM_NORMAL,

	.italic = 0,
	.underline = 0,
	.strikethrough = 0,
};

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
			*c = nl_term_colors[NL_TERM_FAINT][c->ansi];
			break;

		case NL_TERM_COLOR_256:
			// Xterm-256 colors 0..15 are standard and bold colors
			if (c->xterm256 < 8) {
				c = nl_term_colors[NL_TERM_FAINT][c->xterm256];
			} else if (c->xterm256 < 16) {
				c = nl_term_colors[NL_TERM_FAINT][c->xterm256 - 8];
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
			*c = nl_term_colors[NL_TERM_INTENSE][c->ansi];
			break;

		case NL_TERM_COLOR_256:
			if (c->xterm256 < 8) {
				c = nl_term_colors[NL_TERM_INTENSE][c->xterm256];
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
			*c = nl_term_colors[NL_TERM_NORMAL][c->ansi];
			break;

		case NL_TERM_COLOR_256:
			if (c->xterm256 >= 8 && c->xterm256 < 16) {
				c = nl_term_colors[NL_TERM_NORMAL][c->xterm256 - 8];
			}
			break;

		default:
			// Do nothing for RGB colors
			break;
	}
}

static void set_color_intensity(struct nl_term_state *s)
{
	switch(s->intensity) {
		case NL_TERM_NORMAL:
			make_color_normal(&s->foreground);
			break;

		case NL_TERM_INTENSE:
			make_color_intense(&s->foreground);
			break;

		case NL_TERM_FAINT:
			make_color_faint(&s->foreground);
			break;
	}
}

enum color_parse_state {
	EXPECT_CONTROL,
	EXPECT_SEMICOLON,
	EXPECT_2_OR_5,
	EXPECT_XTERM_256,
	EXPECT_RED,
	EXPECT_GREEN,
	EXPECT_BLUE,
};

/*
 * Parses an ANSI color sequence at the start of the given string.  Returns the
 * number of characters consumed, 0 if the sequence could not be parsed as an
 * ANSI color (thus no characters consumed), or -1 on error.
 *
 * The terminal state will be updated with
 *
 * Example valid string prefixes (anything after the m is ignored):
 *     "\e[1m" -- bold
 *     "\e[35m" -- purple/magenta foreground
 *     "\e[38;2;128;128;128m" -- dark gray 24-bit foreground
 */
int nl_parse_ansi_color(char *s, struct nl_term_state *state)
{
	if (CHECK_NULL(s) || CHECK_NULL(state)) {
		return -1;
	}

	struct nl_term_state new_state = *state;

	if (s[0] != 0x1b || s[1] != '[') {
		// Not the beginning of an ANSI escape sequence.
		return 0;
	}

	// TODO: konsole ignores multiple sequential semicolons unless they're within a 256-color or 24-bit-color sequence

	char *ptr = s + 2;
	char *endptr = NULL;
	enum color_parse_state ps = EXPECT_CONTROL;
	enum color_parse_state next_ps = EXPECT_SEMICOLON;
	enum { FOREGROUND_COLOR, BACKGROUND_COLOR } parse_color = FOREGROUND_COLOR;
	unsigned long n;
	while(*ptr) {
		if (*ptr == 'm') {
			// End of sequence; update state and return number of characters in sequence
			*state = new_state;
			return ptr + 1 - s;
		}

		switch(ps) {
			case EXPECT_SEMICOLON:
				if (*ptr != ';') {
					// This should have been a semicolon
					return 0;
				}

			case EXPECT_CONTROL:
				next_ps = EXPECT_SEMICOLON;

				n = strtoul(ptr, &endptr, 10);
				if (endptr == NULL || endptr == ptr) {
					// Failed to parse a number
					return 0;
				}
				ptr = endptr;

				// I bet that some parsers and the original hardware just use the ones digit as a bit index to set or clear for 1..9 and 21..29
				switch(n) {
					case 0:
						new_state = nl_default_term_state;
						break;

					case 1:
						new_state.intensity = NL_TERM_INTENSE;
						set_color_intensity(&new_state);
						break;

					case 2:
						new_state.intensity = NL_TERM_FAINT;
						set_color_intensity(&new_state);
						break;

					case 3:
						new_state.italic = 1;
						break;

					case 4:
						new_state.underline = 1;
						break;

					case 5:
						new_state.blink = 1;
						break;

					case 7:
						new_state.reverse = 1;
						break;

					case 9:
						new_state.strikethrough = 1;
						break;

					case 22:
						new_state.intensity = NL_TERM_NORMAL;
						set_color_intensity(&new_state);
						break;

					case 23:
						new_state.italic = 0;
						break;

					case 24:
						new_state.underline = 0;
						break;

					case 25:
						new_state.blink = 0;
						break;

					case 27:
						new_state.reverse = 0;
						break;

					case 29:
						new_state.strikethrough = 0;
						break;

					case 38:
						ps = EXPECT_SEMICOLON;
						next_ps = EXPECT_2_OR_5;
						parse_color = FOREGROUND_COLOR;
						break;

					case 39:
						new_state.foreground = nl_term_default_foreground;
						set_color_intensity(&new_state);
						break;

					case 48:
						ps = EXPECT_SEMICOLON;
						next_ps = EXPECT_2_OR_5;
						parse_color = FOREGROUND_COLOR;
						break;

					case 49:
						new_state.background = nl_term_default_background;
						break;

					default:
						// Handle standard colors, ignore unsupported values
						if (n >= 30 && n <= 37) {
							new_state.foreground = nl_term_standard_colors[new_state.intensity][n - 30];
						} else if (n >= 40 && n <= 47) {
							new_state.background = nl_term_standard_colors[NL_TERM_COLOR_DEFAULT][n - 30];
						}

						break;
				}

				break;

			case EXPECT_2_OR_5:
				// If we see 2, expect a 24-bit color.  If we see 5, expect an xterm color.
				// For other values, xterm ignores them, while konsole resets the color and treats the
				// value as a standard control code.  Let's do what xterm does.
				n = strtoul(ptr, &endptr, 10);
				if (endptr == NULL || endptr == ptr) {
					// Failed to parse a number
					return 0;
				}

				break;
	}

	// No 'm' was found before the end of the string, so don't consume any characters.
	return 0;
}
