/*
 * Wrapper for the curl (and possibly wget in the future) command.
 * Copyright (C)2015 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#ifndef NLUTILS_URL_REQ_H_
#define NLUTILS_URL_REQ_H_

#include "thread.h"

/*
 * Handle for a url_req context.
 */
struct nl_url_ctx;

/*
 * Result of a request.  Includes success/failure status, the original request
 * URL, headers, response body, error messages, etc.  Passed to request
 * callbacks after a request completes.  Data from this structure should not be
 * referenced outside of or altered by the callback.
 */
struct nl_url_result {
	// Request method
	char method[16];

	// Request URL
	char *url;

	// HTTP response code (e.g. 200)
	int code;

	// Error message, if any (may be set by timeouts or errors)
	char errmsg[1024];

	// Set to 1 if a timeout occurred.  TODO: maybe use negative errno in
	// code to indicate error (EIO) or timeout (ETIMEDOUT)?
	unsigned int timeout:1;

	// Set to 1 if an error occurred.
	unsigned int error:1;

	// Request headers (sent to server)
	struct nl_hash *request_headers;

	// Response headers (received from server)
	struct nl_hash *response_headers;

	// Response body
	struct nl_raw_data response_body;
};

/*
 * Structure for passing parameters to nl_url_req_add().  Use a C99 compound
 * initializer to omit parameters for which the default is acceptable:
 *
 *	ret = nl_url_req_add(
 *		ctx,
 *		callback,
 *		data,
 *		&(struct nl_url_params){ .url = "http://localhost/" }
 *	);
 */
struct nl_url_params {
	// The HTTP method to use (e.g. "GET", "POST").  NULL for "GET".
	char *method;

	// The URL to retrieve (http, https, or ftp).  NULL is not allowed.
	char *url;

	// The request body for POST, PUT, PATCH, etc.  NULL .data for no body.
	//
	// Pass a non-NULL pointer with a size of 0 for an empty body, which
	// will send Content-Length: 0.  The Content-Length header will not be
	// generated automatically, even for request methods like POST and PUT.
	//
	// The request body must be NULL if form parameters are specified with
	// a type other than NL_ON_URL.
	struct nl_raw_data body;

	// A list of extra headers to send with the request.  NULL for no extra
	// headers.
	struct nl_hash *headers;

	// A list of form parameters.  NULL for no form parameters.
	struct nl_hash *form;

	// Form content type, if form is specified.  Default is NL_ON_URL.
	enum {
		NL_ON_URL = 0, // appended to URL (after existing URL parameters)
		NL_URLENCODED = 1, // application/x-www-form-urlencoded sent in body
		NL_MULTIPART = 2, // multipart/form-data sent in body
		NL_FORM_TYPE_MAX = 2,
	} form_type;

	// The timeout for the initial connection, in milliseconds.  Pass 0 for
	// the default of 30000ms (30s).
	int connect_timeout;

	// The timeout for entire request process, in milliseconds.
	// Pass 0 for the default of 30000ms (30s).
	int request_timeout;
};

/*
 * Callback to be called when a URL request completes.  Callbacks are called
 * from a separate thread.
 */
typedef void (*nl_url_callback)(const struct nl_url_result *result, void *data);


/*
 * Initializes a URL request context.  Uses thread_ctx, if given, to create the
 * URL event processing thread.  Returns NULL on error.
 */
struct nl_url_ctx *nl_url_req_init(struct nl_thread_ctx *thread_ctx);

/*
 * Stops the given request context's processing thread, waits for the context's
 * thread to finish (by calling nl_url_req_wait()), then frees the context's
 * associated resources.  This method will cancel any pending requests on the
 * given context.  To complete all requests before shutting down, use
 * nl_url_req_shutdown() followed by nl_url_req_wait().
 */
void nl_url_req_deinit(struct nl_url_ctx *ctx);

/*
 * Asks the request context's processing thread to shut down after all requests
 * complete.  It is possible to add requests after calling this method as long
 * as there are existing requests still waiting to finish, but relying on this
 * is not recommended.
 */
void nl_url_req_shutdown(struct nl_url_ctx *ctx);

/*
 * Waits for the given request context's processing thread to exit (via
 * nl_url_req_shutdown() or nl_url_req_deinit()), then joins it.  Use this
 * method together with nl_url_req_shutdown() to implement a clean shutdown.
 * It is not strictly necessary to call this method, as it is called by
 * nl_url_req_deinit().
 */
void nl_url_req_wait(struct nl_url_ctx *ctx);


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
int nl_url_req_add(struct nl_url_ctx *ctx, nl_url_callback cb, void *cb_data, const struct nl_url_params * const params);



#endif /* NLUTILS_URL_REQ_H_ */
