/*
 * Tests string escape code.
 * Created Oct. 4, 2009, moved from logic system to nlutils Oct. 25, 2011.
 * Copyright (C)2013 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nlutils.h"

char *string_table[] = {
	"This string should have no escapes.",
	"This string should have one escape.\n",
	"This string should have two escapes.\r\n",
	"\tThis one should have: three escapes.\n",
	"Backslashes, too -- \\",
	"Don't forget vertical tabs, part of the ancient line printer leftovers -- \v",
	"Here's all of them -- \f\t:\"\\\v\r\n",
	"This string isn't empty, but the next one is.",
	"",
	":",
};

int count_table[] = {
	0,
	1,
	2,
	3,
	1,
	1,
	8,
	0,
	0,
	1,
};

char *result_table[] = {
	"This string should have no escapes.",
	"This string should have one escape.\\n",
	"This string should have two escapes.\\r\\n",
	"\\tThis one should have\\: three escapes.\\n",
	"Backslashes, too -- \\\\",
	"Don't forget vertical tabs, part of the ancient line printer leftovers -- \\v",
	"Here's all of them -- \\f\\t\\:\\\"\\\\\\v\\r\\n",
	"This string isn't empty, but the next one is.",
	"",
	"\\:",
};

void check_unescape(char *check, char *expect, char *failmessage, int include_zero, int dequote)
{
	char *result;
	int count;

	result = nl_strdup(check);
	DEBUG_OUT("Unescaping '%s', expecting '%s'\n", result, expect);
	count = nl_unescape_string(result, include_zero, dequote);
	if(count != ((int)strlen(check) - (int)strlen(expect)) || strcmp(result, expect)) {
		ERROR_OUT("Unescaping '%s', expecting '%s'\n", result, expect);
		ERROR_OUT("%s\n", failmessage);
		ERROR_OUT("Got: '%s' (%d characters removed, expected %d)\n", result, count, (int)strlen(check) - (int)strlen(expect));
		exit(-1);
	}
	DEBUG_OUT("Result: '%s'\n", result);
	free(result);
}

int main()
{
	char *result;
	int ret;
	size_t i;
	size_t len;

	// Count
	for(i = 0; i < ARRAY_SIZE(string_table); i++) {
		DEBUG_OUT("Checking count for string %zu (%s)\n", i, string_table[i]);
		ret = nl_count_escapes(string_table[i]);
		if(ret != count_table[i]) {
			ERROR_OUT("Fail on count '%s': expected %d, got %d\n", string_table[i], count_table[i], ret);
			return -1;
		}
	}

	// Escape
	for(i = 0; i < ARRAY_SIZE(string_table); i++) {
		DEBUG_OUT("Checking escapes for string %zu (%s)\n", i, string_table[i]);

		result = nl_strdup(string_table[i]);
		len = strlen(string_table[i]) + 1;

		DEBUG_OUT("Old length: %zu\n", len);

		ret = nl_escape_string(&result, &len);

		DEBUG_OUT("Expected: '%s', result: '%s'\n", result_table[i], result);
		DEBUG_OUT("New length: %zu\n", len);

		if(ret || strcmp(result, result_table[i])) {
			ERROR_OUT("Fail on escape: '%s': expected '%s', got '%s'\n", string_table[i], result_table[i], result);
			return -1;
		}

		free(result);
	}

	// Unescape
	for(i = 0; i < ARRAY_SIZE(string_table); i++) {
		DEBUG_OUT("Checking unescapes for string %zu (%s)\n", i, result_table[i]);

		result = nl_strdup(result_table[i]);
		ret = nl_unescape_string(result, 0, 0);

		DEBUG_OUT("Expected: '%s', result: '%s'\n", string_table[i], result);
		DEBUG_OUT("%d characters were claimed removed\n", ret);
		DEBUG_OUT("%zd characters were actually removed\n", strlen(result_table[i]) - strlen(result));

		if(ret < 0 || strcmp(result, string_table[i])) {
			ERROR_OUT("Fail on unescape: '%s': expected '%s', got '%s'\n", result_table[i], string_table[i], result);
			return -1;
		}

		free(result);
	}

	// Escape on oversized buffer
	result = calloc(64, sizeof(char));
	len = 64;
	snprintf(result, 64, "foo\\:bar");
	ret = nl_escape_string(&result, &len);
	if(ret || len != 64 || strcmp(result, "foo\\\\\\:bar")) {
		ERROR_OUT("Fail on oversized escape: got '%s', length %zu\n", result, len);
	}
	free(result);

	// Invalid parameters
	result = NULL;
	if(!nl_escape_string(NULL, &len)) {
		ERROR_OUT("nl_escape_string should've failed for NULL str\n");
		return -1;
	}
	if(!nl_escape_string(&result, &len)) {
		ERROR_OUT("nl_escape_string should've failed for NULL str\n");
		return -1;
	}
	result = nl_strdup("");
	if(!nl_escape_string(&result, NULL)) {
		ERROR_OUT("nl_escape_string should've failed for NULL size\n");
		return -1;
	}
	free(result);
	if(nl_unescape_string(NULL, 0, 0) != -1) {
		ERROR_OUT("nl_unescape_string should've failed for NULL str\n");
		return -1;
	}

	// Invalid escape sequence
	result = nl_strdup("Invalid escape sequence: \\?");
	DEBUG_OUT("Unescaping '%s', expecting '%s' (unchanged)\n", result, result);
	ret = nl_unescape_string(result, 0, 0);
	if(ret != 0 || strcmp(result, "Invalid escape sequence: \\?")) {
		ERROR_OUT("Unescaping '%s', expecting '%s' (unchanged)\n", result, result);
		ERROR_OUT("Failed unescaping a string with an invalid escape sequence\n");
		ERROR_OUT("Got: '%s' (%d characters removed)\n", result, ret);
		return -1;
	}
	DEBUG_OUT("Result: '%s'\n", result);
	free(result);

	// Escape character at end of string
	result = nl_strdup("Escape at end: \\");
	DEBUG_OUT("Unescaping '%s', expecting '%s' (unchanged)\n", result, result);
	ret = nl_unescape_string(result, 0, 0);
	if(ret != 0 || strcmp(result, "Escape at end: \\")) {
		ERROR_OUT("Unescaping '%s', expecting '%s' (unchanged)\n", result, result);
		ERROR_OUT("Failed unescaping a string with escape character at end of string\n");
		ERROR_OUT("Got: '%s' (%d characters removed)\n", result, ret);
		return -1;
	}
	DEBUG_OUT("Result: '%s'\n", result);
	free(result);

	// Hexadecimal escape sequences
	check_unescape("\\x41B\\x43D\\x45F\\x47\\x20\\x42", "ABCDEFG B", "Error unescaping hexadecimal escape sequence", 0, 0);
	check_unescape("Single-character hex escape: '\\xa'", "Single-character hex escape: '\n'", "Error unescaping '\\xa'", 0, 0);
	check_unescape("Invalid hex: '\\xZ4'", "Invalid hex: '\\xZ4'", "Error ignoring invalid hex sequence", 0, 0);
	check_unescape("Hex at end: \\x", "Hex at end: \\x", "Error ignoring incomplete hex sequence at end of string", 0, 0);
	check_unescape("Single at end: \\xd", "Single at end: \r", "Error handling single-character hex sequence at end of string", 0, 0);
	check_unescape("N\\x00u\\x00l\\x00l hex escape (\\\\x00): '\\x00'", "Null hex escape (\\x00): ''", "Error unescaping '\\x00'", 0, 0);

	// Null bytes included
	result = nl_strdup("Nulls are\\x00 okay now.");
	DEBUG_OUT("Unescaping '%s' with nulls included.\n", result);
	ret = nl_unescape_string(result, 1, 0);
	if(ret != 3 || strcmp(result, "Nulls are") || strcmp(result + 10, " okay now.")) {
		ERROR_OUT("Failed unescaping a string with desired null in middle of string\n");
		ERROR_OUT("Got (null not printed): '%s%s' (%d characters removed)\n", result, result + 10, ret);
		ERROR_OUT("Expected (null not printed): '%s%s' (%d characters removed)\n", "Nulls are", " okay now.", 3);
		return -1;
	}
	DEBUG_OUT("Result (null not printed): '%s%s'\n", result, result + 10);
	free(result);

	// Removing quotation marks
	check_unescape("\"Leading \" and trailing but not escaped\\\"\"", "Leading \" and trailing but not escaped\"", "Error removing quotes", 0, 1);
	check_unescape("\"Leading no trailing", "Leading no trailing", "Error removing leading quote", 0, 1);
	check_unescape("Trailing no leading\"", "Trailing no leading\"", "Erroneously removed trailing quote", 0, 1);
	check_unescape("\"Escaped trailing\\\"", "Escaped trailing\"", "Erroneously removed escaped trailing quote", 0, 1);
	check_unescape("\\\"Escaped leading\"", "\"Escaped leading\"", "Error with escaped leading quote", 0, 1);
	check_unescape("\"Escape\\x20with\\x20quotes\"", "Escape with quotes", "Error on quote-conditional unescape", 0, ESCAPE_IF_QUOTED);
	check_unescape("Escape\\x20without\\x20quotes\"", "Escape\\x20without\\x20quotes\"", "Error on quote-conditional unescape", 0, ESCAPE_IF_QUOTED);
	check_unescape("\\\"Escaped\\tquote\\x20without\\x20quotes\"", "\\\"Escaped\\tquote\\x20without\\x20quotes\"", "Error on quote-conditional unescape", 0, ESCAPE_IF_QUOTED);

	INFO_OUT("String escape tests: success\n");

	return 0;
}
