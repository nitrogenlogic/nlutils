/*
 * Wrapper for the curl (and possibly wget in the future) command.
 *
 * The libcurl library shows way too many errors with Valgrind to be used in
 * code that needs to pass Valgrind checks.
 *
 * Modified from curl_stdin.c in the learning_libcurl experiment repository.
 *
 * Copyright (C)2015-2017 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <stddef.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>

// C99 lacks u_char
#define u_char uint8_t
#include <event.h>
#undef u_char

#include "nlutils.h"

// TODO: Support streaming responses instead of a single call to a callback
// (see commit 6b78f209175c4a7bd2ccc59a843390080d46dde1 for a better starting
// point for the read event code)

// TODO: Implement a response size limit, return an error if the limit is exceeded

// TODO: Allow using wget as a backend (wget can be faster)
// See the -O- and -S options to wget (headers are indented by two spaces on stderr)
// Unfortunately wget can't read POST/PUT body data from a pipe or FIFO.

// TODO: Extract a generic libevent+nl_popen3 combination wrapper

// TODO: Add a cancel_req() function?  Would need to provide a way for callers
// to identify requests.

// TODO: Consider supporting a chroot jail for curl/wget process

// TODO: Find a natural way to split this into multiple files?

// TODO: Support specifying the content-type for multipart form data, and
// prevent unwanted content types from sneaking in through form parameters

// TODO: maybe just switch to using libcurl directly and forego process isolation


// Timeout for initial DNS lookup and connection, in milliseconds
#define DEFAULT_CONNECT_TIMEOUT 30000

// Timeout for curl/wget to finish the entire request, in milliseconds
// The read timeout for libevent is based on this value
#define DEFAULT_REQUEST_TIMEOUT 30000

// Timeout for the library to start talking to the curl process, in milliseconds
#define CURL_OPTION_FIFO_TIMEOUT 1000


// Library-global handles for libevent, threads, control pipe, etc.
struct nl_url_req;
struct nl_url_ctx {
	// Thread management
	struct nl_thread_ctx *thread_ctx; // nl_thread thread tracking context
	struct nl_thread *event_thread; // event processing thread
	pthread_mutex_t lock; // struct nl_url_ctx access lock

	// libevent event loop
	struct event_base *evloop;

	// Pipe for sending requests to the event thread
	struct event pipe_ev; // Pipe processing event handle for event loop
	int ev_pipe_readfd; // Read by event thread
	int ev_pipe_writefd; // Written by control functions

	// List of active requests
	struct nl_fifo *reqlist;

	// Library state tracking
	unsigned int running:1; // Set to 1 while event thread is running
	unsigned int stopping:1; // Set to 1 if nl_url_req_stop() has been called
	unsigned int shutdown_when_done:1; // Set to 1 when final request termination should cause loop shutdown
};


// Request-specific internal data
struct nl_url_req {
	struct nl_url_ctx *ctx; // URL request context

	int read_timeout; // libevent read timeout in seconds (based on request timeout)

	nl_url_callback cb; // Completion callback - will be called from event thread with context lock held
	void *cb_data; // User data passed to callback

	// PID of shell that calls curl process
	pid_t pid;

	// File handles and libevent buffers for reading from curl
	struct bufferevent *outbuf;
	struct bufferevent *errbuf;
	int readfd;
	int errfd;

	// Request parameters from the caller (cloned, owned by url_req, freed by free_params())
	struct nl_url_params params;

	// Request results
	struct nl_url_result result;

	// Internal state keeping
	unsigned int has_body:1; // Whether the request has a body (allows zero-sized bodies)
	unsigned int out_eof:1; // Whether the process's STDOUT has encountered EOF
	unsigned int err_eof:1; // Whether the process's STDERR has encountered EOF
};


static void nl_url_req_stop(struct nl_url_ctx *ctx);
static void free_req(struct nl_url_req *req);


// Wraps ctx_lock_impl to provide file/line information for debugging
#define ctx_lock(ctx) do { \
	ctx_lock_impl(ctx, __FILE__, __LINE__); \
} while(0);

// Wraps ctx_unlock_impl to provide file/line information for debugging
#define ctx_unlock(ctx) do { \
	ctx_unlock_impl(ctx, __FILE__, __LINE__); \
} while(0);

// Locks the given struct nl_url_ctx's access lock.  Aborts the application if
// locking fails.
static void ctx_lock_impl(struct nl_url_ctx *ctx, char *file, int line)
{
	int ret;

	DEBUG_OUT("Locking url_req mutex at %s:%d\n", file, line);
	ret = pthread_mutex_lock(&ctx->lock);
	if(ret) {
		ERROR_OUT("Error locking url_req mutex at %s:%d: %s\n", file, line, strerror(ret));
		abort();
	}
}

// Unlocks the given struct nl_url_ctx's access lock.  Aborts the application
// if unlocking fails.
static void ctx_unlock_impl(struct nl_url_ctx *ctx, char *file, int line)
{
	int ret;

	DEBUG_OUT("Unlocking url_req mutex at %s:%d\n", file, line);
	ret = pthread_mutex_unlock(&ctx->lock);
	if(ret) {
		ERROR_OUT("Error unlocking url_req mutex at %s:%d: %s\n", file, line, strerror(ret));
		abort();
	}
}

// Called indirectly by check_process() for each line in the curl/wget output
static int header_line_callback(struct nl_raw_data line, void *cb_data)
{
	struct nl_url_req *req = cb_data;
	struct nl_hash *headers = NULL;
	char *key;
	char *value;

	// TODO: wget support is different here
	if(line.size > 0) {
		if(line.data[0] == '>') {
			headers = req->result.request_headers;
		} else if(line.data[0] == '<') {
			headers = req->result.response_headers;
		}
	}

	// Add header name and value to appropriate list of headers
	if(headers) {
		// Skip first two bytes for "> " or "< ", then find ':'
		value = memchr(line.data + 2, ':', line.size - 2);

		if(value != NULL) {
			key = nl_strndup_term(line.data + 2, value - line.data - 2);
			if(key == NULL) {
				ERROR_OUT("Error duplicating header key.\n");
				return -1;
			}

			// Skip the separating character and any spaces
			value++;
			while(*value == ' ' && value - line.data < (ptrdiff_t)line.size) {
				value++;
			}

			value = nl_strndup_term(value, line.size - (value - line.data));
			if(value == NULL) {
				ERROR_OUT("Error duplicating header %s value.\n", key);
				free(key);
				return -1;
			}

			if(nl_hash_set(headers, key, value)) {
				ERROR_OUT("Error adding header %s to list of headers.\n", key);
				free(key);
				free(value);
				return -1;
			}

			free(key);
			free(value);
		} else if(!nl_strstart(line.data, "< HTTP/") && req->result.code == 0) {
			value = memchr(line.data + 2, ' ', line.size - 2);
			if(value != NULL) {
				// atoi() is OK here because the line can be assumed to be
				// terminated by a newline character or a NUL byte
				req->result.code = atoi(value);
			} else {
				ERROR_OUT("Missing HTTP response code in HTTP response line.\n");
			}
		}
	}

	// TODO: possibly track request state using non-header messages (see
	// git history around these lines for related code)
	//
	// Examples:
	// "* Hostname was NOT found in DNS cache" (indicates resolving)
	// "*   Trying [ip]" (indicates connecting)
	// "* Connected to [hostname] ([ip]) port [port] (#0)" (indicates connection established)

	return 0;
}

// Stores information about the CURL error indicated by retcode into a string.
// The curl(1) manual page lists errors.
static void store_curl_error(char *str, size_t maxlen, int retcode)
{
	char *msg;

	switch(retcode) {
		case 1:
			msg = "Unsupported protocol";
			break;

		case 3:
			msg = "Malformed URL";
			break;

		case 5:
		case 6:
			msg = "Hostname not found";
			break;

		case 7:
		case 35:
			msg = "Connection failed";
			break;

		case 18:
			msg = "Transfer stopped before finished";
			break;

		case 47:
			msg = "Too many HTTP redirects";
			break;

		case 51:
		case 60:
			msg = "Certificate validation failed";
			break;

		case 52:
			msg = "Empty response received";
			break;

		case 55:
		case 56:
			msg = "Network error";
			break;

		case 67:
			msg = "Login failed";
			break;

		default:
			msg = NULL;
			break;
	}

	if(msg) {
		strncpy(str, msg, maxlen);
	} else {
		snprintf(str, maxlen, "Request failed with unknown curl error %d", retcode);
	}
}

// Kills a request's curl process (if any) and waits for it to exit.  Returns
// the return value of nl_wait_get_return(), or 0 if pid was <= 0.  Sets the
// request's pid to 0 afterward.
static int kill_req_and_wait(struct nl_url_req *req)
{
	if(req->pid <= 0) {
		return 0;
	}

	if(kill(req->pid, SIGKILL)) {
		ERRNO_OUT("Error killing request process %ld for %s", (long)req->pid, GUARD_NULL(req->result.url));
	}

	int ret = nl_wait_get_return(req->pid);
	if(ret == -1) {
		ERROR_OUT("Error waiting for request process %ld for %s\n", (long)req->pid, GUARD_NULL(req->result.url));
	}

	req->pid = 0;

	return ret;
}

// Checks for exit and/or EOF from the given request's handling process.  Calls
// the request callback and frees the request if the request has completed.
static void check_process(struct nl_url_req *req)
{
	struct nl_url_ctx *ctx = req->ctx;
	struct evbuffer *stdout_evbuf = EVBUFFER_INPUT(req->outbuf);
	struct evbuffer *stderr_evbuf = EVBUFFER_INPUT(req->errbuf);
	long pid;
	int ret;

	if(req->result.timeout || req->result.error || (req->out_eof && req->err_eof)) {
		DEBUG_OUT("Request %s finished.  Checking status.\n", req->result.url);

		// Ensure the process has exited (e.g. in case of timeout).
		// This works because the child will continue to exist as a
		// zombie until nl_wait_get_return() is called below.
		pid = req->pid;
		ret = kill_req_and_wait(req);
		if(ret == -1) {
			req->result.error = 1;
		} else if(ret < 0) {
			ret = -(ret + 100);
			ERROR_OUT("Request process %ld was killed by signal %d (%s) for %s\n",
					pid, ret, strsignal(ret), req->result.url);
			snprintf(req->result.errmsg, sizeof(req->result.errmsg), "Request interrupted by signal %d (%s)",
					ret, strsignal(ret));
			req->result.error = 1;
		} else if(ret == 28) {
			ERROR_OUT("Request timed out by curl for %s\n", req->result.url);
			snprintf(req->result.errmsg, sizeof(req->result.errmsg), "Request timed out");
			req->result.timeout = 1;
		} else if(ret > 0) {
			store_curl_error(req->result.errmsg, sizeof(req->result.errmsg), ret);
			ERROR_OUT("Request process %ld had curl error (%s) for %s\n",
					pid, req->result.errmsg, req->result.url);
			req->result.error = 1;
		} else {
			DEBUG_OUT("Request process %ld succeeded for %s\n", pid, req->result.url);
		}

#ifdef LIBEVENT_VERSION_NUMBER
		// Temporarily re-enable appending to the evbuffers from libevent2
		evbuffer_unfreeze(stderr_evbuf, 0);
		evbuffer_unfreeze(stdout_evbuf, 0);
#endif /* LIBEVENT_VERSION_NUMBER */

		// Ensure buffers are 0-terminated, then parse headers and save body if successful
		if(evbuffer_add(stderr_evbuf, "", 1) || evbuffer_add(stdout_evbuf, "", 1)) {
			ERROR_OUT("Error adding terminating 0 byte to url_req libevent data buffers\n");
			req->result.error = 1;
		} else {
			// Split header lines
			struct nl_raw_data headers = {
				.data = (char *)EVBUFFER_DATA(stderr_evbuf),
				.size = EVBUFFER_LENGTH(stderr_evbuf) - 1
			};
			nl_split_lines(headers, header_line_callback, req);

			// Store response body
			req->result.response_body.data = (char *)EVBUFFER_DATA(stdout_evbuf);
			req->result.response_body.size = EVBUFFER_LENGTH(stdout_evbuf) - 1;
		}

#ifdef LIBEVENT_VERSION_NUMBER
		// Disable appending to the data buffers in libevent2
		evbuffer_freeze(stderr_evbuf, 0);
		evbuffer_freeze(stdout_evbuf, 0);
#endif /* LIBEVENT_VERSION_NUMBER */

		// Call the request callback, if any
		if(req->cb != NULL) {
			req->cb(&req->result, req->cb_data);
		}

		free_req(req);

		// Shut down the event loop if this was the last request and
		// shutdown was requested.
		ctx_lock(ctx);
		if(ctx->shutdown_when_done && ctx->reqlist->count == 0) {
			DEBUG_OUT("Last request finished; url_req exiting.\n");
			nl_url_req_stop(ctx);
		}
		ctx_unlock(ctx);
	}
}

// Called by libevent when a request process's input fds have an error.
static void bufev_error(struct bufferevent *buf, short errcode, void *cbdata)
{
	struct nl_url_req *req = cbdata;

	switch(errcode) {
		case EVBUFFER_READ | EVBUFFER_EOF:
			if(buf == req->outbuf) {
				req->out_eof = 1;
			} else if(buf == req->errbuf) {
				req->err_eof = 1;
			} else {
				ERROR_OUT("BUG: Unexpected buffer %p in EOF event from request %s\n", buf, req->result.url);
			}
			break;

		case EVBUFFER_READ | EVBUFFER_TIMEOUT:
			if(buf != req->outbuf && buf != req->errbuf) {
				ERROR_OUT("BUG: Unexpected buffer %p in timeout from request %s\n", buf, req->result.url);
			}

			snprintf(req->result.errmsg, sizeof(req->result.errmsg), "Reading data timed out");

			req->result.timeout = 1;
			break;

		default:
			ERROR_OUT("Request %s error on fd %d: 0x%hx\n", req->result.url, req->readfd, errcode);
			req->result.error = 1;
			break;
	}

	check_process(req);
}

/*
 * Asks the request context's processing thread to shut down after all requests
 * complete.  It is possible to add requests after calling this method as long
 * as there are existing requests still waiting to finish, but relying on this
 * is not recommended.
 */
void nl_url_req_shutdown(struct nl_url_ctx *ctx)
{
	if(CHECK_NULL(ctx)) {
		return;
	}

	ctx_lock(ctx);

	if(ctx->ev_pipe_writefd >= 0) {
		struct nl_url_req *z = NULL;
		if(write(ctx->ev_pipe_writefd, &z, sizeof(struct nl_url_req *)) != sizeof(struct nl_url_req *)) {
			ERRNO_OUT("Error writing terminating NULL request to control pipe");
		}
	}

	ctx_unlock(ctx);
}

// Stops the request context's processing thread right away.  Used internally
// by nl_url_req_deinit() and by the event loop's response to
// nl_url_req_shutdown().
static void nl_url_req_stop(struct nl_url_ctx *ctx)
{
	if(CHECK_NULL(ctx)) {
		return;
	}

	ctx_lock(ctx);

	if(ctx->evloop != NULL) {
		if(!ctx->stopping) {
			DEBUG_OUT("Stopping url_req event loop.\n");
			ctx->stopping = 1;
			nl_url_req_shutdown(ctx); // Give the event loop something to process
			event_base_loopexit(ctx->evloop, NULL);
		}
	}

	ctx_unlock(ctx);
}

// Frees the request parameter structure, cloned from user parameters given to
// nl_url_req_add().
static void free_params(struct nl_url_params *params)
{
	// Not freeing params->method and params->url, as they should point to nl_url_result
	params->method = NULL;
	params->url = NULL;

	if(params->body.data) {
		free(params->body.data);
		params->body.size = 0;
		params->body.data = NULL;
	}

	if(params->headers) {
		nl_hash_destroy(params->headers);
	}

	if(params->form) {
		nl_hash_destroy(params->form);
	}
}

// Frees a request's memory, terminates its process, and releases resources.
static void free_req(struct nl_url_req *req)
{
	if(!CHECK_NULL(req)) {
		ctx_lock(req->ctx);

		kill_req_and_wait(req);

		if(req->readfd >= 0 && close(req->readfd)) {
			ERRNO_OUT("Error closing STDOUT for %s", GUARD_NULL(req->result.url));
		}
		if(req->errfd >= 0 && close(req->errfd)) {
			ERRNO_OUT("Error closing STDERR for %s", GUARD_NULL(req->result.url));
		}

		if(nl_fifo_remove(req->ctx->reqlist, req)) {
			ERROR_OUT("Error removing request %s from request list.\n", req->result.url);
		}

		if(req->outbuf) {
			bufferevent_free(req->outbuf);
			req->outbuf = NULL;
		}

		if(req->errbuf) {
			bufferevent_free(req->errbuf);
			req->errbuf = NULL;
		}

		if(req->result.request_headers) {
			nl_hash_destroy(req->result.request_headers);
			req->result.request_headers = NULL;
		}

		if(req->result.response_headers) {
			nl_hash_destroy(req->result.response_headers);
			req->result.response_headers = NULL;
		}

		ctx_unlock(req->ctx);

		free_params(&req->params);

		free(req->result.url);
		free(req);
	}
}


// Writes a key-value option for curl's stdin to the given fd.  Pass
// alternating escape flags and string values as the variable arguments.  If
// the escape flag is nonzero, the following string will be escaped.  The key
// is always escaped.  Pass NULL for the final string to indicate the end of
// the list.
//
// Example to write data=\"@[opt_fifo]\":
//	write_option(writefd, "data", 0, "@", 1, "[opt_fifo]", 0, NULL);
//
// This interface is ugly, but it beats the duplication from before.  An
// alternative might be converting all option writing to use FILE* via
// fdopen().
static int write_option(int fd, char *key, ...)
{
	va_list arglist;

	int escape;
	char *value;

	char *escaped_key = NULL;
	char *escaped_value = NULL;
	size_t key_size = strlen(key) + 1;
	size_t value_size;

	// FIXME: lots of allocation and reallocation going on here, which could be slow
	// FIXME: nl_escape_string() puts backslashes in front of colons for
	// ancient reasons from the logic system serialization format; it
	// doesn't break curl, but is still ugly

	if(CHECK_NULL(escaped_key = strdup(key)) || nl_escape_string(&escaped_key, &key_size)) {
		goto error;
	}

	DEBUG_OUT("\tOption: %s=\"", escaped_key);

	if(write(fd, escaped_key, key_size - 1) != (ssize_t)key_size - 1 || write(fd, "=\"", 2) != 2) {
		goto error;
	}
	free(escaped_key);
	escaped_key = NULL;

	va_start(arglist, key);
	do {
		escape = va_arg(arglist, int);
		value = va_arg(arglist, char *);

		if(value) {
			value_size = strlen(value) + 1;
			escaped_value = strdup(value);
			if(escaped_value == NULL || (escape && nl_escape_string(&escaped_value, &value_size))) {
				goto error;
			}

			DEBUG_OUT_EX("%s", escaped_value);

			if(write(fd, escaped_value, value_size - 1) != (ssize_t)value_size - 1) {
				goto error;
			}

			free(escaped_value);
			escaped_value = NULL;
		}
	} while(value);
	va_end(arglist);

	DEBUG_OUT_EX("\"\n");

	if(write(fd, "\"\n", 2) != 2) {
		goto error;
	}

	return 0;

error:
	ERRNO_OUT("Error writing %s option to CURL stdin", key);

	if(escaped_key) {
		free(escaped_key);
	}
	if(escaped_value) {
		free(escaped_value);
	}

	return -1;
}

// Writes an option with the given key using write_option(), with the given
// milliseconds formatted as a decimal value.
static int write_time_option(int fd, char *key, int milliseconds)
{
	char parambuf[32];
	snprintf(parambuf, sizeof(parambuf), "%d.%03d", milliseconds / 1000, milliseconds % 1000);
	return write_option(fd, key, 0, parambuf, 0, NULL);
}

// Callback for nl_hash_iterate() to write headers to curl's stdin.  Sets
// writefd to -1 to signal failure to nl_url_req_add().
static int header_hash_callback(void *data, char *key, char *value)
{
	int *writeptr = data;
	int writefd = *writeptr;

	if(write_option(writefd, "header", 1, key, 0, ": ", 1, value, 0, NULL)) {
		close(writefd);
		*writeptr = -1;
		return -1;
	}

	return 0;
}

// Callback data for form_hash_callback
struct form_options {
	int *writeptr; // Pointer to writefd
	char *formopt; // String option to use for forms ("form" or "data-binary")
	unsigned int raw:1; // Set to 1 to disable url-encoding of data
};

// Callback for nl_hash_iterate() to write form parameters to curl's stdin.
// Sets writefd to -1 to signal failure to nl_url_req_add().
static int form_hash_callback(void *data, char *key, char *value)
{
	struct form_options *opts = data;
	int writefd = *opts->writeptr;

	char *encoded_key = NULL;
	char *encoded_value = NULL;

	if(opts->raw) {
		encoded_key = key;
		encoded_value = value;
	} else if(CHECK_NULL(encoded_key = nl_url_encode(key, 1, 0)) ||
			CHECK_NULL(encoded_value = nl_url_encode(value, 1, 0))) {
		goto error;
	}

	if(write_option(writefd, opts->formopt, 1, encoded_key, 0, "=", 1, encoded_value, 0, NULL)) {
		goto error;
	}

	if(!opts->raw) {
		free(encoded_key);
		free(encoded_value);
	}

	return 0;

error:
	ERROR_OUT("Error sending form parameter %s to curl\n", key);

	if(encoded_key) {
		free(encoded_key);
	}
	if(encoded_value) {
		free(encoded_value);
	}

	close(writefd);
	*opts->writeptr = -1;
	return -1;
}

// Securely creates a temporary directory and FIFO, storing the paths in string
// pointers pointed to by parameters.  Both paths must be unlinked and their
// strings free()d by the caller.  See fifo(7) and mkfifo(3) for more info.
// TODO: move to stream.c
static int temp_fifo(char **dir, char **fifo)
{
	char *fifo_name = NULL;
	char *dir_name = NULL;
	int dir_created = 0;

	if(CHECK_NULL(dir) || CHECK_NULL(fifo)) {
		return -1;
	}

	dir_name = strdup("/tmp/url.XXXXXX");
	if(dir_name == NULL) {
		ERRNO_OUT("Error allocating temporary directory name buffer");
		goto error;
	}

	fifo_name = malloc(strlen(dir_name) + 10);
	if(fifo_name == NULL) {
		ERRNO_OUT("Error allocating temporary FIFO name buffer");
		goto error;
	}

	if(mkdtemp(dir_name) == NULL) {
		ERRNO_OUT("Error creating temporary directory");
		dir_created = 1;
		goto error;
	}

	snprintf(fifo_name, strlen(dir_name) + 10, "%s/bodyfifo.%04x", dir_name, rand() & 0xffff);

	if(mkfifo(fifo_name, 0600)) {
		ERRNO_OUT("Error creating temporary request body FIFO");
		goto error;
	}

	*dir = dir_name;
	*fifo = fifo_name;

	return 0;

error:
	if(dir_created) {
		rmdir(dir_name);
	}
	if(fifo_name != NULL) {
		free(fifo_name);
	}
	if(dir_name != NULL) {
		free(dir_name);
	}
	return -1;
}

// Sets the current thread's scheduler to SCHED_OTHER and the current process's
// niceness to 0 if it is negative (for nl_url_req_add()).
static void drop_priority_cb(void)
{
	// TODO: These two lines could be a useful function to put in exec.c
	nl_set_thread_priority(NULL, SCHED_OTHER, 0);
	setpriority(PRIO_PROCESS, 0, MAX_NUM(0, getpriority(PRIO_PROCESS, 0)));

	// TODO: drop all possible privileges (need to give the nobody user access to the FIFO)
}

// Clones the given parameter structure into the request structure.  This uses
// more memory and more allocations, but allows curl startup to occur on the
// event thread.
static int clone_params(struct nl_url_req *req, const struct nl_url_params *params)
{
	if(CHECK_NULL(req) || CHECK_NULL(params)) {
		return -1;
	}

	req->params.method = req->result.method;
	req->params.url = req->result.url;

	if(params->body.data) {
		req->has_body = 1;
		req->params.body = nl_copy_data(params->body);
	}

	if(params->headers) {
		req->params.headers = nl_hash_clone(params->headers);
		if(req->params.headers == NULL) {
			ERROR_OUT("Error copying request headers\n");
			return -1;
		}
	}

	if(params->form) {
		req->params.form = nl_hash_clone(params->form);
		if(req->params.form == NULL) {
			ERROR_OUT("Error copying request form parameters\n");
			return -1;
		}
	}

	req->params.form_type = params->form_type;
	req->params.connect_timeout = params->connect_timeout;
	req->params.request_timeout = params->request_timeout;

	return 0;
}

/*
 * Submits a request for background processing by the given request context.
 * If a callback is given, it will be called with the request result and the
 * given callback data when the request completes (or fails).  Pointers to
 * result data should not be held after the callback returns.  See struct
 * nl_url_params for parameter information.  Returns 0 on success, an
 * errno-like value on error.
 *
 * URLs must start with http://, https://, or ftp://.
 */
int nl_url_req_add(struct nl_url_ctx *ctx, nl_url_callback cb, void *cb_data, const struct nl_url_params * const params)
{
	struct nl_url_req *req;

	if(CHECK_NULL(ctx)) {
		return EFAULT;
	}
	if(!ctx->running || ctx->stopping) {
		ERROR_OUT("Cannot add a request to a url_req context that is not running.\n");
		return EINVAL;
	}

	if(CHECK_NULL(params->url)) {
		return EFAULT;
	}
	if(nl_strstart(params->url, "http://") && nl_strstart(params->url, "https://") && nl_strstart(params->url, "ftp://")) {
		ERROR_OUT("URL must start with http://, https://, or ftp://.\n");
		return EINVAL;
	}
	if(params->connect_timeout < 0 || params->request_timeout < 0) {
		ERROR_OUT("Connection and request timeouts in milliseconds must be >= 0\n");
		return EINVAL;
	}
	if(params->body.size != 0 && params->body.data == NULL) {
		ERROR_OUT("Request body length must be zero if request body data is NULL\n");
		return EINVAL;
	}
	if(params->body.data && params->form && params->form_type != NL_ON_URL) {
		ERROR_OUT("Form data type must be NL_ON_URL if a request body is specified\n");
		return EINVAL;
	}
	if((int)params->form_type < 0 || params->form_type > NL_FORM_TYPE_MAX) {
		// ARM treats enums as unsigned, so need to cast to int
		ERROR_OUT("Invalid form type %d\n", params->form_type);
		return EINVAL;
	}


	req = calloc(1, sizeof(struct nl_url_req));
	if(req == NULL) {
		ERRNO_OUT("Error allocating request info structure");
		return ENOMEM;
	}

	ctx_lock(ctx);

	req->ctx = ctx;
	req->cb = cb;
	req->cb_data = cb_data;
	req->readfd = -1;
	req->errfd = -1;

	snprintf(req->result.method, sizeof(req->result.method), "%s", params->method ? params->method : "GET");

	req->result.url = nl_url_encode(params->url, 1, 1);
	if(req->result.url == NULL) {
		ERROR_OUT("Error duplicating/escaping request URL\n");
		goto error;
	}

	if(clone_params(req, params)) {
		ERROR_OUT("Error copying request parameters\n");
		goto error;
	}

	if(nl_fifo_put(ctx->reqlist, req) < 0) {
		ERROR_OUT("Error adding request %s to request list.\n", req->result.url);
		goto error;
	}

	if(write(ctx->ev_pipe_writefd, &req, sizeof(struct nl_url_req *)) != sizeof(struct nl_url_req *)) {
		ERRNO_OUT("Error writing request %s to control pipe", req->result.url);
		goto error;
	}

	DEBUG_OUT("Created %s request to %s\n", req->result.method, req->result.url);

	ctx_unlock(ctx);

	return 0;

error:
	free_req(req);
	return EBUSY;
}

// libevent event handling thread
static void *url_event_thread(void *data)
{
	struct nl_url_ctx *ctx = data;

	// Wait for main thread to finish init function
	ctx_lock(ctx);
	ctx_unlock(ctx);

	// evloop and ctx cannot be freed before here because the shutdown
	// function joins this thread first.  So no need to hold the ctx lock.
	DEBUG_OUT("Starting url_req event loop.\n");
	switch(event_base_dispatch(ctx->evloop)) {
		case 0:
			DEBUG_OUT("Normal url_req event loop exit.\n");
			break;

		case 1:
			ERROR_OUT("BUG: No events were scheduled for URL request event loop.\n");
			break;

		default:
			ERROR_OUT("Error in url_req event loop.\n");
			break;
	}

	ctx_lock(ctx);
	ctx->running = 0;
	ctx_unlock(ctx);

	return NULL;
}

// Compatibility shim for libevent 1.4 through libevent 2.x
// See https://github.com/libevent/libevent/pull/678
static struct bufferevent *create_bufferevent(struct nl_url_req *req, int fd)
{
	struct bufferevent *newbuf;

#if defined(EVENT__NUMERIC_VERSION) && EVENT__NUMERIC_VERSION >= 0x02000000
	// libevent 2 (libevent 2.1 introduced a segfault in bufferevent_new())
	newbuf = bufferevent_socket_new(req->ctx->evloop, fd, 0);
	if(newbuf != NULL) {
		bufferevent_setcb(newbuf, NULL, NULL, bufev_error, req);
	}
#else
	// libevent 1.4
	newbuf = bufferevent_new(fd, NULL, NULL, bufev_error, req);
#endif

	return newbuf;
}

// Helper function to start curl in the control pipe handler (extracted from
// nl_url_req_add()).
static int start_curl(struct nl_url_req *req)
{
	char parambuf[512];
	int writefd = -1;

	char *opt_fifo = NULL;
	char *temp_dir = NULL;
	int bodyfd = -1;

	// Create a FIFO for passing options to curl
	if(temp_fifo(&temp_dir, &opt_fifo)) {
		ERROR_OUT("Error creating option-passing FIFO for %s\n", req->params.url);
		goto error;
	}

	DEBUG_OUT("/usr/bin/curl -K %s --compressed -s -v -X %s -H Expect: -H \"Connection: close\" -H Transfer-Encoding:\n", opt_fifo, req->result.method);

	// Start the curl process (see the curl manual page)
	// -K -- read options from FIFO
	// -s -- silent (disables progress meter)
	// -v -- verbose (enables verbose output without progress meter)
	req->pid = nl_popen3vec(
			&bodyfd, &req->readfd, &req->errfd,
			"/usr/bin/curl",
			(char *[]){
				"/usr/bin/curl",
				"-K", opt_fifo,
				"--compressed", // Send Accept-Encoding, decode gzip/deflate/etc.
				"-s",
				"-v",
				"-X", req->result.method,
				"-H", "Expect:",
				"-H", "Connection: close",
				"-H", "Transfer-Encoding:",
				NULL
			},
			environ,
			drop_priority_cb
			);
	if(req->pid <= 0) {
		ERROR_OUT("Error starting request process for %s.\n", req->params.url);
		goto error;
	}

	// TODO: it could be nice to check pending requests on a timer, instead
	// of waiting for curl to open the option fifo.  But it would be even
	// better to switch to using libcurl directly.

	// Open option-passing FIFO
	writefd = nl_open_timeout(
			opt_fifo,
			O_WRONLY | O_CLOEXEC,
			0,
			(struct timespec){.tv_nsec = CURL_OPTION_FIFO_TIMEOUT * 1000000}
			);
	if(writefd < 0) {
		ERROR_OUT("Error opening option-passing FIFO for writing: %s\n", strerror(-writefd));
		goto error;
	}

	if(write_option(writefd, "url", 1, req->result.url, 0, NULL)) {
		goto error;
	}

	// Send timeouts to curl
	int connect_timeout = req->params.connect_timeout > 0 ? req->params.connect_timeout : DEFAULT_CONNECT_TIMEOUT;
	int request_timeout = req->params.request_timeout > 0 ? req->params.request_timeout : DEFAULT_REQUEST_TIMEOUT;
	if(write_time_option(writefd, "connect-timeout", connect_timeout) ||
			write_time_option(writefd, "max-time", request_timeout)) {
		ERRNO_OUT("Error writing timeouts to CURL stdin for %s", req->result.url);
		goto error;
	}

	req->read_timeout = MAX_NUM(5, (request_timeout + 1999) / 1000);

	// Send form data to curl
	if(req->params.form) {
		char *formopt = req->params.form_type == NL_MULTIPART ? "form" : "data-binary";
		unsigned int encoded = req->params.form_type == NL_MULTIPART;

		if(req->params.form_type == NL_ON_URL && write_option(writefd, "get", 0, NULL)) {
			goto error;
		}

		nl_hash_iterate(req->params.form, form_hash_callback, &(struct form_options){&writefd, formopt, encoded});

		if(writefd == -1) {
			ERROR_OUT("Error passing form data to curl\n");
			goto error;
		}
	}

	// Send request headers to curl
	if(req->params.headers) {
		nl_hash_iterate(req->params.headers, header_hash_callback, &writefd);

		if(writefd == -1) {
			ERROR_OUT("Error passing request headers to curl\n");
			goto error;
		}
	}

	// Prepare curl to receive a request body
	if(req->has_body) {
		snprintf(parambuf, sizeof(parambuf), "%zd", req->params.body.size);
		if(header_hash_callback(&writefd, "Content-Length", parambuf)) {
			goto error;
		}

		if(write_option(writefd, "upload-file", 0, "-", 0, NULL)) {
			goto error;
		}
	}

	// Close option-passing FIFO so curl starts reading request body (if provided)
	if(close(writefd)) {
		ERRNO_OUT("Error closing option-passing FIFO for %s", req->result.url);
		goto error;
	}
	writefd = -1;

	// Write request body, close request body fd
	if(req->has_body && req->params.body.data) {
		if(nl_write_stream(bodyfd, &req->params.body)) {
			ERROR_OUT("Error writing request body for %s", req->result.url);
			goto error;
		}
	}
	if(close(bodyfd)) {
		ERRNO_OUT("Error closing request body fd for %s", req->result.url);
		goto error;
	}
	bodyfd = -1;

	// Clean up the option-passing FIFO
	if(unlink(opt_fifo)) {
		ERRNO_OUT("Error deleting request body FIFO %s for %s", opt_fifo, req->result.url);
	}
	free(opt_fifo);
	opt_fifo = NULL;
	if(rmdir(temp_dir)) {
		ERRNO_OUT("Error deleting temporary directory %s for %s", temp_dir, req->result.url);
	}
	free(temp_dir);
	temp_dir = NULL;

	// Connect curl output to libevent
	req->outbuf = create_bufferevent(req, req->readfd);
	if(req->outbuf == NULL) {
		ERROR_OUT("Error creating bufferevent for request %s stdout.\n", req->result.url);
		goto error;
	}

	req->errbuf = create_bufferevent(req, req->errfd);
	if(req->errbuf == NULL) {
		ERROR_OUT("Error creating bufferevent for request %s stderr.\n", req->result.url);
		goto error;
	}

	// Allocate space for headers as logged by curl
	req->result.request_headers = nl_hash_create();
	if(req->result.request_headers == NULL) {
		ERROR_OUT("Error allocating request headers hash.\n");
		goto error;
	}

	req->result.response_headers = nl_hash_create();
	if(req->result.response_headers == NULL) {
		ERROR_OUT("Error allocating response headers hash.\n");
		goto error;
	}

	return 0;

error:
	req->result.error = 1;

	if(writefd > -1) {
		close(writefd);
	}
	if(bodyfd > -1) {
		close(bodyfd);
	}
	if(opt_fifo != NULL) {
		unlink(opt_fifo);
		free(opt_fifo);
	}
	if(temp_dir != NULL) {
		rmdir(temp_dir);
		free(temp_dir);
	}

	// TODO: Notify callback of failed request

	// Read any errors from CURL
	// TODO: Pass these errors to the caller somehow?
	kill_req_and_wait(req);
	if(req->errfd > -1) {
		char err_buf[512];
		int ret;
		nl_set_nonblock(req->errfd, 1);
		do {
			ret = read(req->errfd, err_buf, sizeof(err_buf));
			if(ret > 0) {
				ERROR_OUT("Error output from CURL: ");
				fwrite(err_buf, 1, ret, stderr);
				ERROR_OUT_EX("\n");
			}
		} while(ret > 0);
	}

	free_req(req);
	return EBUSY;
}

// Control pipe request handler; reads request pointers from control pipe,
// starts request processes.
static void control_pipe_handler(int fd, short evtype, void *cbdata)
{
	struct nl_url_ctx *ctx = cbdata;
	union {
		// TODO: Reading pointers from a pipe is kind of weird;
		// consider alternatives
		//
		// Could store request info as an array, then pass an array
		// offset, but modification would need to be prevented between
		// write and read.
		//
		// Could also store an ID in the structure, then scan the list
		// of requests for the ID written to the pipe.
		//
		// Could pass request info structure (or a simplified
		// equivalent) directly, perform all initialization in this
		// function, and write a result back to the pipe so the caller
		// of nl_url_req_add() still gets a return status (or just use the
		// request callback if an error occurs).
		char data[sizeof(struct nl_url_req *)];
		struct nl_url_req *req;
	} rip; // request info pointer, rest in peace/good luck (strict aliasing), rip (as in tear)
	size_t off;
	ssize_t len;

	(void)evtype; // unused parameter

	off = 0;
	do {
		len = read(fd, &rip.data[off], sizeof(struct nl_url_req *) - off);
		if(len == -1) {
			ERRNO_OUT("Error reading from control pipe");
		} else if(len > 0) {
			off += len;
		}
	} while(len > 0 && off < sizeof(struct nl_url_req *));

	if(off < sizeof(struct nl_url_req *)) {
		ERROR_OUT("Couldn't read entire pointer from control pipe.  Got %zd of %zu bytes.\n",
				off, sizeof(struct nl_url_req *));
		return;
	}

	ctx_lock(ctx);

	if(rip.req == NULL) {
		ctx->shutdown_when_done = 1;

		if(ctx->reqlist->count == 0) {
			nl_url_req_stop(ctx);
		}

		ctx_unlock(ctx);
		return;
	}

	ctx_unlock(ctx);

	// ctx cannot be freed while the event thread is running, so no need to
	// hold the ctx lock while starting curl.
	if(start_curl(rip.req)) {
		ERROR_OUT("Error starting curl for %s\n", rip.req->result.url);
		goto error;
	}

	if(bufferevent_base_set(ctx->evloop, rip.req->outbuf)) {
		ERROR_OUT("Error assigning request %s stdout bufferevent to event loop.\n", rip.req->result.url);
		goto error;
	}
	bufferevent_settimeout(rip.req->outbuf, rip.req->read_timeout, 0);
	if(bufferevent_enable(rip.req->outbuf, EV_READ)) {
		ERROR_OUT("Error enabling read events on request %s stdout bufferevent.\n", rip.req->result.url);
		goto error;
	}

	if(bufferevent_base_set(ctx->evloop, rip.req->errbuf)) {
		ERROR_OUT("Error assigning request %s stderr bufferevent to event loop.\n", rip.req->result.url);
		goto error;
	}
	bufferevent_settimeout(rip.req->errbuf, rip.req->read_timeout, 0);
	if(bufferevent_enable(rip.req->errbuf, EV_READ)) {
		ERROR_OUT("Error enabling read events on request %s stderr bufferevent.\n", rip.req->result.url);
		goto error;
	}

	DEBUG_OUT("Started %s request to %s\n", rip.req->result.method, rip.req->result.url);

	return;

error:
	if(rip.req) {
		free_req(rip.req);
	}
}


/*
 * Initializes a URL request context.  Uses thread_ctx, if given, to create the
 * URL event processing thread.  Returns NULL on error.
 */
struct nl_url_ctx *nl_url_req_init(struct nl_thread_ctx *thread_ctx)
{
	struct nl_url_ctx *ctx;
	int pipefds[2];
	int ret;

	// FIXME: deallocate structure on init error

	ctx = calloc(1, sizeof(struct nl_url_ctx));
	if(ctx == NULL) {
		ERRNO_OUT("Error allocating url_req context structure");
		return NULL;
	}

	ctx->reqlist = nl_fifo_create();
	if(CHECK_NULL(ctx->reqlist)) {
		free(ctx);
		return NULL;
	}

	ctx->evloop = event_base_new();
	if(CHECK_NULL(ctx->evloop)) {
		nl_fifo_destroy(ctx->reqlist);
		free(ctx);
		return NULL;
	}
	DEBUG_OUT("Using libevent's %s polling method\n", event_base_get_method(ctx->evloop));

	// Create access lock
	ret = nl_create_mutex(&ctx->lock, -1);
	if(ret) {
		ERROR_OUT("Error creating thread locking mutex: %s\n", strerror(ret));
		return NULL;
	}

	ctx_lock(ctx);


	// Create control pipe
	ret = pipe(pipefds);
	if(ret) {
		ERRNO_OUT("Error creating control pipe");
		return NULL;
	}
	ctx->ev_pipe_readfd = pipefds[0];
	ctx->ev_pipe_writefd = pipefds[1];

	if(nl_set_cloexec(ctx->ev_pipe_readfd) || nl_set_cloexec(ctx->ev_pipe_writefd)) {
		ERRNO_OUT("Error setting close-on-execute flag on control pipe");
		return NULL;
	}

	// Add event handlers for control pipe
	event_set(&ctx->pipe_ev, ctx->ev_pipe_readfd, EV_PERSIST | EV_READ, control_pipe_handler, ctx);
	if(event_base_set(ctx->evloop, &ctx->pipe_ev)) {
		ERROR_OUT("Error assigning event loop to control pipe event.\n");
		return NULL;
	}
	if(event_add(&ctx->pipe_ev, NULL)) {
		ERROR_OUT("Error adding control pipe event to the event loop.\n");
		return NULL;
	}


	// Create a threading context if one was not provided
	if(thread_ctx == NULL) {
		thread_ctx = nl_create_thread_context();
		if(thread_ctx == NULL) {
			ERROR_OUT("Error creating url_req thread context.\n");
			return NULL;
		}
		ctx->thread_ctx = thread_ctx;
	}

	ctx->running = 1;

	// Create the event handling thread
	ret = nl_create_thread(thread_ctx, NULL, url_event_thread, ctx, "url_req events", &ctx->event_thread);
	if(ret) {
		ERROR_OUT("Error creating url_req event thread: %s\n", strerror(ret));
		return NULL;
	}

	ret = nl_set_thread_priority(ctx->event_thread, SCHED_OTHER, 0);
	if(ret) {
		ERROR_OUT("Warning: unable to set url_req event thread to SCHED_OTHER: %s\n", strerror(ret));
	}

	ctx_unlock(ctx);

	return ctx;
}

/*
 * Waits for the given request context's processing thread to exit (via
 * nl_url_req_shutdown() or nl_url_req_deinit()), then joins it.  Use this
 * method together with nl_url_req_shutdown() to implement a clean shutdown.
 * It is not strictly necessary to call this method, as it is called by
 * nl_url_req_deinit().
 */
void nl_url_req_wait(struct nl_url_ctx *ctx)
{
	int ret;

	if(CHECK_NULL(ctx)) {
		return;
	}

	// TODO: Add a shutdown timeout that kills the thread if it takes too
	// long to exit?

	if(ctx->event_thread != NULL) {
		ret = nl_join_thread(ctx->event_thread, NULL);
		if(ret) {
			ERROR_OUT("Error joining url_req event thread: %s\n", strerror(ret));
		}
		ctx->event_thread = NULL;
	}
}

/*
 * Stops the given request context's processing thread, waits for the context's
 * thread to finish (by calling nl_url_req_wait()), then frees the context's
 * associated resources.  This method will cancel any pending requests on the
 * given context.  To complete all requests before shutting down, use
 * nl_url_req_shutdown() followed by nl_url_req_wait().
 */
void nl_url_req_deinit(struct nl_url_ctx *ctx)
{
	int ret;

	if(CHECK_NULL(ctx)) {
		return;
	}

	ctx_lock(ctx);
	if(ctx->running && ctx->evloop != NULL) {
		nl_url_req_stop(ctx);

		if(ctx->event_thread != NULL) {
			// Must unlock so event thread can acquire lock to
			// process remaining events and shut down
			ctx_unlock(ctx);
			nl_url_req_wait(ctx);
			ctx_lock(ctx);
		}
	}

	if(event_initialized(&ctx->pipe_ev) && event_del(&ctx->pipe_ev)) {
		ERROR_OUT("Error removing URL request control pipe event.\n");
	}

	if(ctx->evloop != NULL) {
		struct event_base *evloop = ctx->evloop;
		ctx->evloop = NULL;

		ctx_unlock(ctx);
		event_base_free(evloop);
		ctx_lock(ctx);
	}

	if(ctx->reqlist != NULL) {
		const struct nl_fifo_element *iter = NULL;
		struct nl_url_req *req = NULL, *prev = NULL;

		if(ctx->reqlist->count > 0) {
			INFO_OUT("Warning: url_req shut down with %d pending requests\n", ctx->reqlist->count);
		}

		do {
			prev = req;
			req = nl_fifo_next(ctx->reqlist, &iter);
			if(prev != NULL) {
				free_req(prev);
			}
		} while(req != NULL);

		nl_fifo_destroy(ctx->reqlist);
		ctx->reqlist = NULL;
	}

	if(close(ctx->ev_pipe_readfd)) {
		ERRNO_OUT("Error closing read side of control pipe");
	}
	if(close(ctx->ev_pipe_writefd)) {
		ERRNO_OUT("Error closing write side of control pipe");
	}

	ctx_unlock(ctx);

	if(ctx->thread_ctx) {
		nl_destroy_thread_context(ctx->thread_ctx);
	}

	ret = pthread_mutex_destroy(&ctx->lock);
	if(ret) {
		ERROR_OUT("Error destroying thread lock: %s\n", strerror(ret));
	}

	free(ctx);
}
