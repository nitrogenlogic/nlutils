/*
 * Tests URL request (url_req) functions.
 * Copyright (C)2015-2017 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdio.h>
#include <stdlib.h>

#define u_char uint8_t
#include <event.h>
#undef u_char

#include "nlutils.h"

struct url_req_test {
	char *desc; // Test description

	struct nl_url_params params; // Request parameters

	unsigned int passed:1; // Set to 1 by callback if test passed
	unsigned int failed:1; // Set to 1 if test failed (used if passed gets set by callback after an earlier check fails)
	unsigned int expect_error:1; // 1 to expect an error in result, 0 to expect success
	unsigned int expect_timeout:1; // 1 to expect a timeout
	unsigned int expect_add_error:1; // 1 to expect error adding request

	// Alternating key/value strings, NULL key to end
	char **headers; // Request headers to convert to form used by nl_url_params
	char **form; // Form parameters to convert to form used by nl_url_params

	int expect_code; // HTTP code to expect (0 to ignore)
	char *expect_errmsg; // String to expect in error message (NULL to ignore)

	size_t *expect_body_size; // Size of the response body to expect (NULL to ignore)
	char **expect_body; // Strings to expect in response body (end with NULL)
};

#define BASE_URL "http://localhost:38212"

static struct url_req_test req_tests[] = {
	// Request tests
	{
		.desc = "Simple GET request",
		.params = {
			.url = BASE_URL,
		},
		.expect_body = (char *[]){
			"GET:",
			NULL
		},
	},
	{
		.desc = "POST, no data",
		.params = {
			.method = "POST",
			.url = BASE_URL,
			.body = { .size = 0, .data = "" },
		},
		.expect_body = (char *[]){
			"POST:",
			"Content-Length: 0",
			NULL
		},
	},
	{
		.desc = "POST, no data, with headers",
		.params = {
			.method = "POST",
			.url = BASE_URL,
			.body = { .size = 0, .data = "" },
		},
		.headers = (char *[]){
			"X-Test-Header", "12345",
			NULL, NULL
		},
		.expect_body = (char *[]){
			"POST:",
			"Content-Length: 0",
			"X-Test-Header: 12345",
			NULL
		},
	},
	{
		.desc = "POST with data",
		.params = {
			.method = "POST",
			.url = BASE_URL "/reverse",
			.body = { .size = 10, .data = "Logic\t\r\n\vP" },
		},
		.expect_body_size = &(size_t){10},
		.expect_body = (char *[]){
			"P\v\n\r\tcigoL",
			NULL
		},
	},
	{
		.desc = "POST expecting zero-size response",
		.params = {
			.method = "POST",
			.url = BASE_URL "/reverse",
			.body = { .size = 0, .data = "" },
		},
		.expect_body_size = &(size_t){0},
		.expect_body = (char *[]){
			NULL
		},
	},
	{
		.desc = "DELETE request",
		.params = {
			.method = "DELETE",
			.url = BASE_URL,
		},
		.expect_body = (char *[]){
			"DELETE:",
			NULL
		},
	},
	{
		.desc = "PUT with data",
		.params = {
			.method = "PUT",
			.url = BASE_URL,
			.body = { .size = 4, .data = "Test" },
		},
		.expect_body = (char *[]){
			"PUT:",
			NULL
		},
	},
	{
		.desc = "GET with headers",
		.params = {
			.url = BASE_URL,
		},
		.headers = (char *[]){
			"X-Test-Header", "12345",
			NULL, NULL
		},
		.expect_body = (char *[]){
			"GET:",
			"X-Test-Header: 12345",
			NULL
		},
	},
	{
		.desc = "PATCH with headers",
		.params = {
			.method = "PATCH:",
			.url = BASE_URL,
			.body = { .size = 3, .data = "\t\vP" },
		},
		.headers = (char *[]){
			"X-Patch-Test", "54321",
			NULL, NULL
		},
		.expect_body = (char *[]){
			"PATCH:",
			"X-Patch-Test: 54321",
			"\t\vP",
			NULL
		},
	},
	{
		.desc = "GET with URL form",
		.params = {
			.url = BASE_URL "/?q=9",
		},
		.headers = (char *[]){
			"X-Form-Test", "Tested",
			NULL, NULL
		},
		.form = (char *[]){
			"a", "1",
			"b", "2+2=4",
			NULL, NULL
		},
		.expect_body = (char *[]){
			"GET:",
			"X-Form-Test: Tested",
			"?q=9&a=1&b=2%2b2%3d4",
			NULL
		},
	},
	{
		.desc = "POST with URL form",
		.params = {
			.method = "POST",
			.url = BASE_URL "/?q=9",
			.body = { .size = 0, .data = "" },
		},
		.headers = (char *[]){
			"X-Form-Test", "Tested",
			NULL, NULL
		},
		.form = (char *[]){
			"a", "1",
			"b", "2+2=4",
			NULL, NULL
		},
		.expect_body = (char *[]){
			"POST:",
			"X-Form-Test: Tested",
			"?q=9&a=1&b=2%2b2%3d4",
			NULL
		},
	},
	{
		.desc = "POST with URL-encoded body form",
		.params = {
			.method = "POST",
			.url = BASE_URL "/?q=9",
			.form_type = NL_URLENCODED,
		},
		.headers = (char *[]){
			"X-Form-Test", "Tested",
			NULL, NULL
		},
		.form = (char *[]){
			"a", "1",
			"b", "2+2=4",
			NULL, NULL
		},
		.expect_body = (char *[]){
			"POST:",
			"X-Form-Test: Tested",
			"Content-Type: application/x-www-form-urlencoded",
			"?q=9 HTTP",
			"a=1&b=2%2b2%3d4",
			NULL
		},
	},
	{
		.desc = "POST with multipart body form",
		.params = {
			.method = "POST",
			.url = BASE_URL "/?q=9",
			.form_type = NL_MULTIPART,
		},
		.headers = (char *[]){
			"X-Form-Test", "Tested",
			NULL, NULL
		},
		.form = (char *[]){
			"a", "1",
			"b", "2+2=4",
			NULL, NULL
		},
		.expect_body = (char *[]){
			"POST:",
			"X-Form-Test: Tested",
			"Content-Type: multipart/form-data",
			"?q=9 HTTP",
			"name=\"a\"",
			"1\r\n",
			"name=\"b\"",
			"2+2=4\r\n",
			NULL
		},
	},
	{
		.desc = "GET with empty form and headers",
		.params = {
			.url = BASE_URL,
		},
		.headers = (char *[]){
			NULL, NULL
		},
		.form = (char *[]){
			NULL, NULL
		},
		.expect_body = (char *[]){
			"GET:",
			NULL
		},
	},
	{
		.desc = "Delayed request, no timeout",
		.params = {
			.url = BASE_URL "/delayed",
		},
		.expect_timeout = 0,
		.expect_body = (char *[]){
			"Delayed",
			NULL
		},
	},

	// Connection problem tests
	{
		.desc = "Connection timeout",
		.params = {
			// FIXME: This can't be guaranteed not to connect
			.url = "http://169.254.253.218:45454",
			.connect_timeout = 1, // 1ms
		},
		.expect_timeout = 1,
		.expect_errmsg = "timed out",
	},
	{
		.desc = "Request timeout",
		.params = {
			.url = BASE_URL "/delayed",
			.request_timeout = 100, // 100ms
		},
		.expect_timeout = 1,
		.expect_errmsg = "timed out",
	},
	{
		.desc = "Connection failure",
		.params = {
			.url = "http://localhost:61236/",
		},
		.expect_error = 1,
		.expect_errmsg = "onnection",
	},

	// Data processing tests
	{
		.desc = "URL escaping",
		.params = {
			.url = BASE_URL "/space ,tab\t,circle-R®\n",
		},
		.expect_body = (char *[]){
			"space%20,tab%09,circle-R%c2%ae%0a",
			NULL
		},
	},

	// Invalid argument tests
	{
		.desc = "Invalid URL",
		.params = {
			.url = "invalid",
		},
		.expect_add_error = 1,
	},
	{
		.desc = "No URL",
		.expect_add_error = 1,
	},
	{
		.desc = "Invalid form type",
		.params = {
			.url = BASE_URL,
			.form_type = 10,
		},
		.expect_add_error = 1,
	},
	{
		.desc = "Invalid connection timeout",
		.params = {
			.url = BASE_URL,
			.connect_timeout = -1,
		},
		.expect_add_error = 1,
	},
	{
		.desc = "Invalid request timeout",
		.params = {
			.url = BASE_URL,
			.request_timeout = -1,
		},
		.expect_add_error = 1,
	},
};

static int header_iterate(void *cb_data, char *key, char *value)
{
	(void)cb_data; // unused parameter

	fputs("\t\t", stderr);
	fputs(key, stderr);
	fputs(": ", stderr);
	puts(value);

	return 0;
}

static void test_url_cb(const struct nl_url_result *result, void *data)
{
	struct url_req_test *test = data;

	DEBUG_OUT("Verifying test: %s\n", test->desc);

	if(test->expect_error != result->error) {
		ERROR_OUT("Expected %s error, got %s error on %s\n",
				test->expect_error ? "an" : "no",
				result->error ? "an" : "no",
				test->desc);
		test->failed = 1;
	}

	if(test->expect_timeout != result->timeout) {
		ERROR_OUT("Expected %s timeout, got %s timeout on %s\n",
				test->expect_timeout ? "an" : "no",
				result->timeout ? "an" : "no",
				test->desc);
		test->failed = 1;
	}

	if(test->expect_code && test->expect_code != result->code) {
		ERROR_OUT("Expected HTTP code %d, got %d on %s\n",
				test->expect_code, result->code, test->desc);
		test->failed = 1;
	}

	if(test->expect_errmsg && *test->expect_errmsg && !strstr(result->errmsg, test->expect_errmsg)) {
		ERROR_OUT("Expected error message to contain '%s', got '%s' on %s\n",
				test->expect_errmsg, result->errmsg, test->desc);
		test->failed = 1;
	}

	if(test->expect_body) {
		if(result->response_body.data == NULL) {
			ERROR_OUT("Expected body, but body data was NULL on %s\n", test->desc);
			test->failed = 1;
		} else {
			for(char **ptr = test->expect_body; *ptr; ptr++) {
				if(!strstr(result->response_body.data, *ptr)) {
					ERROR_OUT("Expected body to contain '%s' on %s\n", *ptr, test->desc);
					test->failed = 1;
				}

				if(result->response_body.size < strlen(*ptr)) {
					ERROR_OUT("Expected body to be at least %zu bytes for '%s' on %s\n", strlen(*ptr), *ptr, test->desc);
					test->failed = 1;
				}
			}
		}
	}

	if(test->expect_body_size) {
		if(result->response_body.size != *test->expect_body_size) {
			ERROR_OUT("Expected body size %zu, got %zu on %s\n",
					*test->expect_body_size,
					result->response_body.size,
					test->desc);
			test->failed = 1;
		}
	}

	if(!test->failed) {
		test->passed = 1;
	} else {
		fprintf(stderr, "\n\nData from failed test %s:\n", test->desc);

		// TODO: Delete unneeded printouts
		fprintf(stderr, "\tURL: %s\n", result->url);
		fprintf(stderr, "\tTimeout: %d\n", result->timeout);
		fprintf(stderr, "\tError: %d\n", result->error);
		fprintf(stderr, "\tError message: %s\n", result->errmsg);
		fprintf(stderr, "\tResponse code: %d\n", result->code);

		fprintf(stderr, "\tRequest headers (%zu):\n", result->request_headers->count);
		nl_hash_iterate(result->request_headers, header_iterate, NULL);
		fprintf(stderr, "\tResponse headers (%zu):\n", result->response_headers->count);
		nl_hash_iterate(result->response_headers, header_iterate, NULL);

		fprintf(stderr, "\tResponse body (%zu byte(s)):\n", result->response_body.size);
		fprintf(stderr, "-------------------\n");
		fwrite(result->response_body.data, result->response_body.size, 1, stderr);
		fprintf(stderr, "-------------------\n\n");
	}
}

// Turns pairs of strings in an array into an nl_hash, stopping at a NULL key
// or value.  TODO: Move into hash.c?  Also add a varargs hash creation func?
static struct nl_hash *create_hash_from_strings(char **strings)
{
	struct nl_hash *h;

	if(CHECK_NULL(h = nl_hash_create())) {
		abort();
	}

	while(strings[0] && strings[1]) {
		if(nl_hash_set(h, strings[0], strings[1])) {
			ERROR_OUT("Error building a hash for passing test options\n");
			abort();
		}
		strings += 2;
	}

	return h;
}

// Adds a test request to the request queue
static void add_test(struct nl_url_ctx *ctx, struct url_req_test *test)
{
	int ret;

	DEBUG_OUT("Adding test '%s'\n", test->desc);

	if(test->headers) {
		test->params.headers = create_hash_from_strings(test->headers);
	}
	if(test->form) {
		test->params.form = create_hash_from_strings(test->form);
	}

	ret = nl_url_req_add(ctx, test_url_cb, test, &test->params);
	if(ret && !test->expect_add_error) {
		ERROR_OUT("Error adding test '%s': %s\n", test->desc, strerror(ret));
		test->failed = 1;
	} else if(!ret && test->expect_add_error) {
		ERROR_OUT("Expected an error adding test '%s', but got no error\n", test->desc);
		test->failed = 1;
	} else if(test->expect_add_error && ret) {
		test->passed = 1;
	}

	if(test->params.headers) {
		nl_hash_destroy(test->params.headers);
		test->params.headers = NULL;
	}
	if(test->params.form) {
		nl_hash_destroy(test->params.form);
		test->params.form = NULL;
	}
}

// Checks a test request's result (returns nonzero for fail, 0 for pass)
static int check_test(struct url_req_test *test)
{
	if(test->failed || !test->passed) {
		ERROR_OUT("Test '%s' failed (failed=%u, passed=%u).\n",
				test->desc, test->failed, test->passed);
		return -1;
	}

	return 0;
}

// Callback for log messages from libevent
void libevent_log(int severity, const char *msg)
{
	static const char * const sev_strs[] = {
		"debug",
		"info",
		"warning",
		"error",
	};

	if(severity >= _EVENT_LOG_WARN) {
		ERROR_OUT("libevent %s: %s\n", sev_strs[CLAMP(0, (int)ARRAY_SIZE(sev_strs) - 1, severity)], msg);
	}
}

int main(void)
{
	struct nl_url_ctx *ctx[2];
	struct nl_thread_ctx *threads;
	int ret;
	size_t i;

	if(CHECK_NULL(threads = nl_create_thread_context())) {
		return -1;
	}

	// Make sure libevent log messages are displayed
	INFO_OUT("libevent version: %s\n", event_get_version());
	event_set_log_callback(libevent_log);

	// Test with two contexts to verify thread context handling
	INFO_OUT("Creating two URL request contexts to test thread handling\n");
	if(CHECK_NULL(ctx[0] = nl_url_req_init(NULL)) || CHECK_NULL(ctx[1] = nl_url_req_init(threads))) {
		return -1;
	}

	// Submit tests to url_req thread
	for(i = 0; i < ARRAY_SIZE(req_tests); i++) {
		add_test(ctx[i & 1], &req_tests[i]);
	}

	INFO_OUT("All tests submitted.  Requesting shutdown when done.\n");
	nl_url_req_shutdown(ctx[0]);
	nl_url_req_shutdown(ctx[1]);

	INFO_OUT("Waiting for url_req to shut down.\n");
	nl_url_req_wait(ctx[0]);
	nl_url_req_wait(ctx[1]);

	INFO_OUT("Cleaning url_req.\n");
	nl_url_req_deinit(ctx[1]);
	nl_url_req_deinit(ctx[0]);

	INFO_OUT("Destroying thread context.\n");
	nl_destroy_thread_context(threads);

	INFO_OUT("Verifying tests.\n");
	for(i = 0, ret = 0; i < ARRAY_SIZE(req_tests); i++) {
		if(check_test(&req_tests[i])) {
			ret++;
		}
	}

	INFO_OUT("Testing startup and shutdown without adding requests.\n");
	if(CHECK_NULL(ctx[0] = nl_url_req_init(NULL))) {
		return -1;
	}
	nl_url_req_deinit(ctx[0]);

	INFO_OUT("Testing startup, wait, and shutdown without adding requests.\n");
	if(CHECK_NULL(ctx[0] = nl_url_req_init(NULL))) {
		return -1;
	}
	nl_url_req_shutdown(ctx[0]);
	nl_url_req_wait(ctx[0]);
	nl_url_req_deinit(ctx[0]);

	if(ret) {
		ERROR_OUT("%d of %zu url_req tests failed\n", ret, ARRAY_SIZE(req_tests));
	} else {
		INFO_OUT("All url_req tests passed.\n");
	}

	return ret;
}
