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
 */

#include "parser.h"
#include "main.h"
#include "log.h"
#include "utils.h"
#include <event2/http.h>
#include <event2/buffer.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct debug_instance_t {
	int configured;
	char* http_ip;
	uint16_t http_port;
	struct evhttp* http_server;
} debug_instance;

static debug_instance instance = {
	.configured = 0,
};

static parser_entry debug_entries[] =
{
	{ .key = "http_ip", .type = pt_pchar },
	{ .key = "http_port", .type = pt_uint16 },
	{ }
};

static int debug_onenter(parser_section *section)
{
	if (instance.configured) {
		parser_error(section->context, "only one instance of debug is valid");
		return -1;
	}
	memset(&instance, 0, sizeof(instance));
	for (parser_entry *entry = &section->entries[0]; entry->key; entry++)
		entry->addr =
			(strcmp(entry->key, "http_ip") == 0) ? (void*)&instance.http_ip:
			(strcmp(entry->key, "http_port") == 0) ? (void*)&instance.http_port :
			NULL;
	return 0;
}

static int debug_onexit(parser_section *section)
{
	instance.configured = 1;
	if (!instance.http_ip)
		instance.http_ip = strdup("localhost");
	return 0;
}

static parser_section debug_conf_section =
{
	.name    = "debug",
	.entries = debug_entries,
	.onenter = debug_onenter,
	.onexit  = debug_onexit,
};

static void debug_pipe(struct evhttp_request *req, void *arg)
{
	UNUSED(arg);
	int fds[2];
	int code = (pipe(fds) == 0) ? HTTP_OK : HTTP_SERVUNAVAIL;
	evhttp_send_reply(req, code, NULL, NULL);
}
static void debug_meminfo_json(struct evhttp_request *req, void *arg)
{
	UNUSED(arg);

	struct evbuffer* body = evbuffer_new();
	evbuffer_add(body, "{", 1);
	
	FILE* fd = fopen("/proc/vmstat", "r");
	while (!feof(fd)) {
		char buf[64];
		size_t pages;
		if (fscanf(fd, "%63s %zu", buf, &pages) == 2 && strncmp(buf, "nr_", 3) == 0) {
			evbuffer_add_printf(body, "\"%s\": %zu, ", buf, pages);
		}
		for (int c = 0; c != EOF && c != '\n'; c = fgetc(fd))
			;
	}
	fclose(fd);

	size_t vmsize, rss, share, text, z, data;
	fd = fopen("/proc/self/statm", "r");
	if (fscanf(fd, "%zu %zu %zu %zu %zu %zu %zu", &vmsize, &rss, &share, &text, &z, &data, &z) == 7) {
		evbuffer_add_printf(body,
			"\"vmsize\": %zu, \"vmrss\": %zu, \"share\": %zu, \"text\": %zu, \"data\": %zu, ",
			vmsize, rss, share, text, data);
		
	}
	fclose(fd);
	evbuffer_add_printf(body, "\"getpagesize\": %d}", getpagesize());

	evhttp_send_reply(req, HTTP_OK, NULL, body);
	evbuffer_free(body);
}

static int debug_fini();

static int debug_init(struct event_base* evbase)
{
	if (!instance.http_port)
		return 0;

	instance.http_server = evhttp_new(evbase);
	if (!instance.http_server) {
		log_errno(LOG_ERR, "evhttp_new()");
		goto fail;
	}

	if (evhttp_bind_socket(instance.http_server, instance.http_ip, instance.http_port) != 0) {
		log_errno(LOG_ERR, "evhttp_bind_socket()");
		goto fail;
	}

	if (evhttp_set_cb(instance.http_server, "/debug/pipe", debug_pipe, NULL) != 0) {
		log_errno(LOG_ERR, "evhttp_set_cb()");
		goto fail;
	}

	if (evhttp_set_cb(instance.http_server, "/debug/meminfo.json", debug_meminfo_json, NULL) != 0) {
		log_errno(LOG_ERR, "evhttp_set_cb()");
		goto fail;
	}

	return 0;

fail:
	debug_fini();
	return -1;
}

static int debug_fini()
{
	if (instance.http_server) {
		evhttp_free(instance.http_server);
		instance.http_server = 0;
	}
	free(instance.http_ip);
	memset(&instance, 0, sizeof(instance));
	return 0;
}

app_subsys debug_subsys =
{
	.init = debug_init,
	.fini = debug_fini,
	.conf_section = &debug_conf_section,
};

/* vim:set tabstop=4 softtabstop=4 shiftwidth=4: */
/* vim:set foldmethod=marker foldlevel=32 foldmarker={,}: */
