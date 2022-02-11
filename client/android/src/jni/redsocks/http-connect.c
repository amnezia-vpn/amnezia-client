/* redsocks - transparent TCP-to-proxy redirector
 * Copyright (C) 2007-2011 Leonid Evdokimov <leon@darkk.net.ru>
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 *
 * http-connect upstream module for redsocks
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "log.h"
#include "redsocks.h"
#include "http-auth.h"

typedef enum httpc_state_t {
	httpc_new,
	httpc_request_sent,
	httpc_reply_came, // 200 OK came, skipping headers...
	httpc_headers_skipped, // starting pump!
	httpc_no_way, // proxy can't handle the request
	httpc_MAX,
} httpc_state;


#define HTTP_HEAD_WM_HIGH 4096  // that should be enough for one HTTP line.


static void httpc_client_init(redsocks_client *client)
{
	client->state = httpc_new;
}

static void httpc_instance_fini(redsocks_instance *instance)
{
	http_auth *auth = red_http_auth(instance);
	free(auth->last_auth_query);
	auth->last_auth_query = NULL;
}

static struct evbuffer *httpc_mkconnect(redsocks_client *client);

extern const char *auth_request_header;
extern const char *auth_response_header;

static void httpc_read_cb(struct bufferevent *buffev, void *_arg)
{
	redsocks_client *client = _arg;

	assert(client->relay == buffev);
	assert(client->state == httpc_request_sent || client->state == httpc_reply_came);

	redsocks_touch_client(client);

	// evbuffer_add() triggers callbacks, so we can't write to client->client
	// till we know that we're going to ONFAIL_FORWARD_HTTP_ERR.
	// And the decision is made when all the headers are processed.
	struct evbuffer* tee = NULL;
	const bool do_errtee = client->instance->config.on_proxy_fail == ONFAIL_FORWARD_HTTP_ERR;

	if (client->state == httpc_request_sent) {
		size_t len = evbuffer_get_length(buffev->input);
		char *line = redsocks_evbuffer_readline(buffev->input);
		if (line) {
			unsigned int code;
			if (sscanf(line, "HTTP/%*u.%*u %u", &code) == 1) { // 1 == one _assigned_ match
				if (code == 407) { // auth failed
					http_auth *auth = red_http_auth(client->instance);

					if (auth->last_auth_query != NULL && auth->last_auth_count == 1) {
						redsocks_log_error(client, LOG_NOTICE, "HTTP Proxy auth failed: %s", line);
						client->state = httpc_no_way;
					} else if (client->instance->config.login == NULL || client->instance->config.password == NULL) {
						redsocks_log_error(client, LOG_NOTICE, "HTTP Proxy auth required, but no login/password configured: %s", line);
						client->state = httpc_no_way;
					} else {
						if (do_errtee)
							tee = evbuffer_new();
						char *auth_request = http_auth_request_header(buffev->input, tee);
						if (!auth_request) {
							redsocks_log_error(client, LOG_NOTICE, "HTTP Proxy auth required, but no <%s> header found: %s", auth_request_header, line);
							client->state = httpc_no_way;
						} else {
							free(line);
							if (tee)
								evbuffer_free(tee);
							free(auth->last_auth_query);
							char *ptr = auth_request;

							ptr += strlen(auth_request_header);
							while (isspace(*ptr))
								ptr++;

							size_t last_auth_query_len = strlen(ptr) + 1;
							auth->last_auth_query = calloc(last_auth_query_len, 1);
							memcpy(auth->last_auth_query, ptr, last_auth_query_len);
							auth->last_auth_count = 0;

							free(auth_request);

							if (bufferevent_disable(client->relay, EV_WRITE)) {
								redsocks_log_errno(client, LOG_ERR, "bufferevent_disable");
								return;
							}

							/* close relay tunnel */
							redsocks_bufferevent_free(client->relay);

							/* set to initial state*/
							client->state = httpc_new;

							/* and reconnect */
							redsocks_connect_relay(client);
							return;
						}
					}
				} else if (200 <= code && code <= 299) {
					client->state = httpc_reply_came;
				} else {
					redsocks_log_error(client, LOG_NOTICE, "HTTP Proxy error: %s", line);
					client->state = httpc_no_way;
				}
			} else {
				redsocks_log_error(client, LOG_NOTICE, "HTTP Proxy bad firstline: %s", line);
				client->state = httpc_no_way;
			}
			if (do_errtee && client->state == httpc_no_way) {
				if (bufferevent_write(client->client, line, strlen(line)) != 0 ||
				    bufferevent_write(client->client, "\r\n", 2) != 0)
				{
					redsocks_log_errno(client, LOG_NOTICE, "bufferevent_write");
					goto fail;
				}
			}
			free(line);
		}
		else if (len >= HTTP_HEAD_WM_HIGH) {
			redsocks_log_error(client, LOG_NOTICE, "HTTP Proxy reply is too long, %zu bytes", len);
			client->state = httpc_no_way;
		}
	}

	if (do_errtee && client->state == httpc_no_way) {
		if (tee) {
			if (bufferevent_write_buffer(client->client, tee) != 0) {
				redsocks_log_errno(client, LOG_NOTICE, "bufferevent_write_buffer");
				goto fail;
			}
		}
		redsocks_shutdown(client, client->client, SHUT_RD);
		const size_t avail = evbuffer_get_length(client->client->input);
		if (avail) {
			if (evbuffer_drain(client->client->input, avail) != 0) {
				redsocks_log_errno(client, LOG_NOTICE, "evbuffer_drain");
				goto fail;
			}
		}
		redsocks_shutdown(client, client->relay, SHUT_WR);
		client->state = httpc_headers_skipped;
	}

fail:
	if (tee) {
		evbuffer_free(tee);
	}

	if (client->state == httpc_no_way) {
		redsocks_drop_client(client);
		return;
	}

	while (client->state == httpc_reply_came) {
		char *line = redsocks_evbuffer_readline(buffev->input);
		if (line) {
			if (strlen(line) == 0) {
				client->state = httpc_headers_skipped;
			}
			free(line);
		}
		else {
			break;
		}
	}

	if (client->state == httpc_headers_skipped) {
		redsocks_start_relay(client);
	}
}

static struct evbuffer *httpc_mkconnect(redsocks_client *client)
{
	struct evbuffer *buff = NULL, *retval = NULL;
	char *auth_string = NULL;
	int len;

	buff = evbuffer_new();
	if (!buff) {
		redsocks_log_errno(client, LOG_ERR, "evbuffer_new");
		goto fail;
	}

	http_auth *auth = red_http_auth(client->instance);
	++auth->last_auth_count;

	const char *auth_scheme = NULL;

	if (auth->last_auth_query != NULL) {
		/* find previous auth challange */

		if (strncasecmp(auth->last_auth_query, "Basic", 5) == 0) {
			auth_string = basic_authentication_encode(client->instance->config.login, client->instance->config.password);
			auth_scheme = "Basic";
		} else if (strncasecmp(auth->last_auth_query, "Digest", 6) == 0) {
			/* calculate uri */
			char uri[128];
			snprintf(uri, 128, "%s:%u", inet_ntoa(client->destaddr.sin_addr), ntohs(client->destaddr.sin_port));

			/* prepare an random string for cnounce */
			char cnounce[17];
			snprintf(cnounce, sizeof(cnounce), "%08x%08x", red_randui32(), red_randui32());

			auth_string = digest_authentication_encode(auth->last_auth_query + 7, //line
					client->instance->config.login, client->instance->config.password, //user, pass
					"CONNECT", uri, auth->last_auth_count, cnounce); // method, path, nc, cnounce
			auth_scheme = "Digest";
		}
	}

	// TODO: do accurate evbuffer_expand() while cleaning up http-auth
	len = evbuffer_add_printf(buff, "CONNECT %s:%u HTTP/1.0\r\n",
		inet_ntoa(client->destaddr.sin_addr),
		ntohs(client->destaddr.sin_port));
	if (len < 0) {
		redsocks_log_errno(client, LOG_ERR, "evbufer_add_printf");
		goto fail;
	}

	if (auth_string) {
		len = evbuffer_add_printf(buff, "%s %s %s\r\n",
			auth_response_header, auth_scheme, auth_string);
		if (len < 0) {
			redsocks_log_errno(client, LOG_ERR, "evbufer_add_printf");
			goto fail;
		}
		free(auth_string);
		auth_string = NULL;
	}

	const enum disclose_src_e disclose_src = client->instance->config.disclose_src;
	if (disclose_src != DISCLOSE_NONE) {
		char clientip[INET_ADDRSTRLEN];
		const char *ip = inet_ntop(client->clientaddr.sin_family, &client->clientaddr.sin_addr, clientip, sizeof(clientip));
		if (!ip) {
			redsocks_log_errno(client, LOG_ERR, "inet_ntop");
			goto fail;
		}
		if (disclose_src == DISCLOSE_X_FORWARDED_FOR) {
			len = evbuffer_add_printf(buff, "X-Forwarded-For: %s\r\n", ip);
		} else if (disclose_src == DISCLOSE_FORWARDED_IP) {
			len = evbuffer_add_printf(buff, "Forwarded: for=%s\r\n", ip);
		} else if (disclose_src == DISCLOSE_FORWARDED_IPPORT) {
			len = evbuffer_add_printf(buff, "Forwarded: for=\"%s:%d\"\r\n", ip,
				ntohs(client->clientaddr.sin_port));
		}
		if (len < 0) {
			redsocks_log_errno(client, LOG_ERR, "evbufer_add_printf");
			goto fail;
		}
	}

	len = evbuffer_add(buff, "\r\n", 2);
	if (len < 0) {
		redsocks_log_errno(client, LOG_ERR, "evbufer_add");
		goto fail;
	}

	retval = buff;
	buff = NULL;

fail:
	if (auth_string)
		free(auth_string);
	if (buff)
		evbuffer_free(buff);
	return retval;
}


static void httpc_write_cb(struct bufferevent *buffev, void *_arg)
{
	redsocks_client *client = _arg;

	redsocks_touch_client(client);

	if (client->state == httpc_new) {
		redsocks_write_helper_ex(
			buffev, client,
			httpc_mkconnect, httpc_request_sent, 1, HTTP_HEAD_WM_HIGH
			);
	}
	else if (client->state >= httpc_request_sent) {
		bufferevent_disable(buffev, EV_WRITE);
	}
}


relay_subsys http_connect_subsys =
{
	.name                 = "http-connect",
	.payload_len          = 0,
	.instance_payload_len = sizeof(http_auth),
	.readcb               = httpc_read_cb,
	.writecb              = httpc_write_cb,
	.init                 = httpc_client_init,
	.instance_fini        = httpc_instance_fini,
};

/* vim:set tabstop=4 softtabstop=4 shiftwidth=4: */
/* vim:set foldmethod=marker foldlevel=32 foldmarker={,}: */
