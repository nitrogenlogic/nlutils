/*
 * Uses url_req to GET a URL given on the command line.
 * Copyright (C)2015 Mike Bourgeous.  Released under AGPLv3 in 2018.
 */
#include <stdio.h>

#include "nlutils.h"

static void url_callback(const struct nl_url_result *result, void *data)
{
	(void)data; // unused parameter

	fwrite(result->response_body.data, result->response_body.size, 1, stdout);
}

int main(int argc, char *argv[])
{
	struct nl_url_ctx *ctx;
	int ret;

	if(argc != 2) {
		printf("Usage: %s url\n", argv[0]);
		return 1;
	}

	if(CHECK_NULL(ctx = nl_url_req_init(NULL))) {
		return -1;
	}

	ret = nl_url_req_add(ctx, url_callback, NULL, &(struct nl_url_params){.url = argv[1]});
	if(ret) {
		ERROR_OUT("Error starting request: %s\n", strerror(ret));
		nl_url_req_deinit(ctx);
		return -1;
	}

	nl_url_req_shutdown(ctx);
	nl_url_req_wait(ctx);
	nl_url_req_deinit(ctx);

	return 0;
}
