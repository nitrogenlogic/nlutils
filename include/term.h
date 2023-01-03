/*
 * Terminal-related utility functions, such as ANSI color escape parsing.
 *
 * Written in part as an exercise.  There are probably off-the-shelf tools that
 * one should use instead.
 *
 * Do not include this file directly.  Instead, include nlutils.h.
 *
 * Copyright (C)2023 Mike Bourgeous, licensed under AGPLv3.
 */
#ifndef NLUTILS_TERM_H_
#define NLUTILS_TERM_H_

#include <stdint.h>

/*
 * The type or origin of a color stored in struct nl_term_color.  This is
 * determined by the escape sequence that generated the color.
 */
enum nl_term_color_type {
	// A default color set by 0m, 39m, or 49m, that does not match any of the standard colors
	NL_TERM_COLOR_DEFAULT = -1,

	// A standard color set by 30..37m or 40..47m
	NL_TERM_COLOR_STANDARD = 0,

	// An Xterm-256 color set by 38;5;Xm
	NL_TERM_COLOR_XTERM256 = 1,

	// A 24-bit RGB color set by 38;2;R;G;Bm
	NL_TERM_COLOR_RGB = 2,
};

/*
 * Represents a foreground or background color, including information on
 * whether it originated as a standard ANSI color or an Xterm 8-bit color.
 *
 * Bold/intensity is represented in struct nl_term_state, though Xterm colors 8
 * through 15 also represent intense colors.
 */
struct nl_term_color {
	// 24-bit renderable color (always set, regardless of whether 24-bit color was used)
	// Foreground: \e[38;2;R;G;Bm
	// Background: \e[48;2;R;G;Bm
	uint8_t r, g, b;

	// Xterm 256 color index, if current color was set using standard or xterm 256 color, otherwise 0
	// Foreground: \e[38;5;Xm
	// Background: \e[48;5;Xm
	uint8_t xterm256;

	// ANSI color index, if current color was set using standard colors, otherwise 0
	// Foreground: \e[30..37m
	// Background: \e[40..47m
	uint8_t ansi:3;

	// The source of this color.
	enum nl_term_color_type color_type;
};

/*
 * Represents the font intensity (bold/intense, faint, or normal).
 * Bold: \e[1m
 * Faint: \e[2m
 * Normal: \e[22m
 */
enum nl_term_intensity {
	NL_TERM_NORMAL = 0,
	NL_TERM_INTENSE = 1,
	NL_TERM_BOLD = 1,
	NL_TERM_FAINT = 2,
};

/*
 * Represents an ANSI color state.  You may use nl_init_term_state() or the
 * nl_default_term_state constant to set typical default values.
 */
struct nl_term_state {
	// Foreground color
	struct nl_term_color fg;

	// Background color
	struct nl_term_color bg;

	// Normal, bold, or faint
	enum nl_term_intensity intensity;

	// Font style flags
	unsigned int italic:1;
	unsigned int underline:1;
	unsigned int blink:1;
	unsigned int reverse:1;
	unsigned int strikethrough:1;
};

/* Default terminal state, for use as an initializer. */
extern const struct nl_term_state nl_default_term_state;

/*
 * The default foreground color if no explicit color is set and neither bold
 * nor faint intensity is active.
 */
extern const struct nl_term_color nl_term_default_foreground;

/*
 * The default bold foreground color if no explicit color is set but color
 * intensity is set to bold.
 */
extern const struct nl_term_color nl_term_bold_foreground;

/*
 * The default faint foreground color if no explicit color is set, but color
 * intensity is set to faint.
 */
extern const struct nl_term_color nl_term_faint_foreground;

/*
 * Struct initializer with default foreground color for struct nl_term_state.
 */
#define NL_TERM_FOREGROUND_INITIALIZER { \
	.r = 158, .g = 158, .b = 158, \
	.xterm256 = 7, \
	.ansi = 7, \
	.color_type = NL_TERM_COLOR_DEFAULT, \
}

/*
 * Struct initializer with default background color for struct nl_term_state.
 */
#define NL_TERM_BACKGROUND_INITIALIZER { \
	.r = 16, .g = 16, .b = 16, \
	.xterm256 = 0, \
	.ansi = 0, \
	.color_type = NL_TERM_COLOR_DEFAULT, \
}

/*
 * Default values for struct nl_term_state.
 * Use as nl_term_state s = NL_TERM_STATE_INITIALIZER;
 */
#define NL_TERM_STATE_INITIALIZER {\
	.fg = NL_TERM_FOREGROUND_INITIALIZER, \
	.bg = NL_TERM_BACKGROUND_INITIALIZER, \
	.intensity = NL_TERM_NORMAL, \
	.italic = 0, \
	.underline = 0, \
	.blink = 0, \
	.reverse = 0, \
	.strikethrough = 0, \
}

/*
 * Sets default state values on the given terminal state struct.
 */
void nl_init_term_state(struct nl_term_state *s);

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
int nl_term_parse_ansi_color(char *s, struct nl_term_state *state);

#endif /* NLUTILS_TERM_H_ */
