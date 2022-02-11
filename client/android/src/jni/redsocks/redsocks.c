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

#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <event.h>
#include "list.h"
#include "parser.h"
#include "log.h"
#include "main.h"
#include "base.h"
#include "redsocks.h"
#include "utils.h"
#include "libevent-compat.h"


#define REDSOCKS_RELAY_HALFBUFF  4096

enum pump_state_t {
	pump_active = -1,
	pump_MAX = 0,
};

static const char *redsocks_event_str(unsigned short what);
static int redsocks_start_bufferpump(redsocks_client *client);
static int redsocks_start_splicepump(redsocks_client *client);

static void redsocks_conn_list_del(redsocks_client *client);

extern relay_subsys http_connect_subsys;
extern relay_subsys http_relay_subsys;
extern relay_subsys socks4_subsys;
extern relay_subsys socks5_subsys;
static relay_subsys *relay_subsystems[] =
{
	&http_connect_subsys,
	&http_relay_subsys,
	&socks4_subsys,
	&socks5_subsys,
};

static list_head instances = LIST_HEAD_INIT(instances);

// Managing connection pressure.
static uint32_t redsocks_conn;
static uint32_t accept_backoff_ms;
static struct event accept_backoff_ev;

static parser_entry redsocks_entries[] =
{
	{ .key = "local_ip",   .type = pt_in_addr },
	{ .key = "local_port", .type = pt_uint16 },
	{ .key = "ip",         .type = pt_in_addr },
	{ .key = "port",       .type = pt_uint16 },
	{ .key = "type",       .type = pt_pchar },
	{ .key = "login",      .type = pt_pchar },
	{ .key = "password",   .type = pt_pchar },
	{ .key = "listenq",    .type = pt_uint16 },
	{ .key = "splice",     .type = pt_bool },
	{ .key = "disclose_src", .type = pt_disclose_src },
	{ .key = "on_proxy_fail", .type = pt_on_proxy_fail },
	{ }
};

static bool is_splice_good()
{
	struct utsname u;
	if (uname(&u) != 0) {
		return false;
	}

	unsigned long int v[4] = { 0, 0, 0, 0 };
	char *rel = u.release;
	for (int i = 0; i < SIZEOF_ARRAY(v); ++i) {
		v[i] = strtoul(rel, &rel, 0);
		while (*rel && !isdigit(*rel))
			++rel;
	}

	// haproxy assumes that splice "works" for 2.6.27.13+
	return (v[0] > 2) ||
	       (v[0] == 2 && v[1] > 6) ||
	       (v[0] == 2 && v[1] == 6 && v[2] > 27) ||
	       (v[0] == 2 && v[1] == 6 && v[2] == 27 && v[3] >= 13);
}

static int redsocks_onenter(parser_section *section)
{
	// FIXME: find proper way to calulate instance_payload_len
	int instance_payload_len = 0;
	relay_subsys **ss;
	FOREACH(ss, relay_subsystems)
		if (instance_payload_len < (*ss)->instance_payload_len)
			instance_payload_len = (*ss)->instance_payload_len;

	redsocks_instance *instance = calloc(1, sizeof(*instance) + instance_payload_len);
	if (!instance) {
		parser_error(section->context, "Not enough memory");
		return -1;
	}

	INIT_LIST_HEAD(&instance->list);
	INIT_LIST_HEAD(&instance->clients);
	instance->config.bindaddr.sin_family = AF_INET;
	instance->config.bindaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	instance->config.relayaddr.sin_family = AF_INET;
	instance->config.relayaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	/* Default value can be checked in run-time, but I doubt anyone needs that.
	 * Linux:   sysctl net.core.somaxconn
	 * FreeBSD: sysctl kern.ipc.somaxconn */
	instance->config.listenq = SOMAXCONN;
	instance->config.use_splice = is_splice_good();
	instance->config.disclose_src = DISCLOSE_NONE;
	instance->config.on_proxy_fail = ONFAIL_CLOSE;

	for (parser_entry *entry = &section->entries[0]; entry->key; entry++)
		entry->addr =
			(strcmp(entry->key, "local_ip") == 0)   ? (void*)&instance->config.bindaddr.sin_addr :
			(strcmp(entry->key, "local_port") == 0) ? (void*)&instance->config.bindaddr.sin_port :
			(strcmp(entry->key, "ip") == 0)         ? (void*)&instance->config.relayaddr.sin_addr :
			(strcmp(entry->key, "port") == 0)       ? (void*)&instance->config.relayaddr.sin_port :
			(strcmp(entry->key, "type") == 0)       ? (void*)&instance->config.type :
			(strcmp(entry->key, "login") == 0)      ? (void*)&instance->config.login :
			(strcmp(entry->key, "password") == 0)   ? (void*)&instance->config.password :
			(strcmp(entry->key, "listenq") == 0)    ? (void*)&instance->config.listenq :
			(strcmp(entry->key, "splice") == 0)     ? (void*)&instance->config.use_splice :
			(strcmp(entry->key, "disclose_src") == 0) ? (void*)&instance->config.disclose_src :
			(strcmp(entry->key, "on_proxy_fail") == 0) ? (void*)&instance->config.on_proxy_fail :
			NULL;
	section->data = instance;
	return 0;
}

static int redsocks_onexit(parser_section *section)
{
	/* FIXME: Rewrite in bullet-proof style. There are memory leaks if config
	 *        file is not correct, so correct on-the-fly config reloading is
	 *        currently impossible.
	 */
	redsocks_instance *instance = section->data;

	section->data = NULL;
	for (parser_entry *entry = &section->entries[0]; entry->key; entry++)
		entry->addr = NULL;

	instance->config.bindaddr.sin_port = htons(instance->config.bindaddr.sin_port);
	instance->config.relayaddr.sin_port = htons(instance->config.relayaddr.sin_port);

	if (instance->config.type) {
		relay_subsys **ss;
		FOREACH(ss, relay_subsystems) {
			if (!strcmp((*ss)->name, instance->config.type)) {
				instance->relay_ss = *ss;
				list_add(&instance->list, &instances);
				break;
			}
		}
		if (!instance->relay_ss) {
			parser_error(section->context, "invalid `type` <%s> for redsocks", instance->config.type);
			return -1;
		}
	}
	else {
		parser_error(section->context, "no `type` for redsocks");
		return -1;
	}

	if (instance->config.disclose_src != DISCLOSE_NONE && instance->relay_ss != &http_connect_subsys) {
		parser_error(section->context, "only `http-connect` supports `disclose_src` at the moment");
		return -1;
	}

	if (instance->config.on_proxy_fail != ONFAIL_CLOSE && instance->relay_ss != &http_connect_subsys) {
		parser_error(section->context, "only `http-connect` supports `on_proxy_fail` at the moment");
		return -1;
	}

	return 0;
}

static parser_section redsocks_conf_section =
{
	.name    = "redsocks",
	.entries = redsocks_entries,
	.onenter = redsocks_onenter,
	.onexit  = redsocks_onexit
};

void redsocks_log_write_plain(
		const char *file, int line, const char *func, int do_errno,
		const struct sockaddr_in *clientaddr, const struct sockaddr_in *destaddr,
		int priority, const char *orig_fmt, ...
) {
	if (!should_log(priority))
		return;

	int saved_errno = errno;
	struct evbuffer *fmt = evbuffer_new();
	va_list ap;
	char clientaddr_str[RED_INET_ADDRSTRLEN], destaddr_str[RED_INET_ADDRSTRLEN];

	if (!fmt) {
		log_errno(LOG_ERR, "evbuffer_new()");
		// no return, as I have to call va_start/va_end
	}

	if (fmt) {
		evbuffer_add_printf(fmt, "[%s->%s]: %s",
				red_inet_ntop(clientaddr, clientaddr_str, sizeof(clientaddr_str)),
				red_inet_ntop(destaddr, destaddr_str, sizeof(destaddr_str)),
				orig_fmt);
	}

	va_start(ap, orig_fmt);
	if (fmt) {
		errno = saved_errno;
		_log_vwrite(file, line, func, do_errno, priority, (const char*)evbuffer_pullup(fmt, -1), ap);
		evbuffer_free(fmt);
	}
	va_end(ap);
}

void redsocks_touch_client(redsocks_client *client)
{
	redsocks_gettimeofday(&client->last_event);
}

static bool shut_both(redsocks_client *client)
{
	return client->relay_evshut == (EV_READ|EV_WRITE) && client->client_evshut == (EV_READ|EV_WRITE);
}

static int bufprio(redsocks_client *client, struct bufferevent *buffev)
{
	// client errors are logged with LOG_INFO, server errors with LOG_NOTICE
	return (buffev == client->client) ? LOG_INFO : LOG_NOTICE;
}

static const char* bufname(redsocks_client *client, struct bufferevent *buf)
{
	assert(buf == client->client || buf == client->relay);
	return buf == client->client ? "client" : "relay";
}

static void redsocks_relay_readcb(redsocks_client *client, struct bufferevent *from, struct bufferevent *to)
{
	if (evbuffer_get_length(to->output) < to->wm_write.high) {
		if (bufferevent_write_buffer(to, from->input) == -1)
			redsocks_log_errno(client, LOG_ERR, "bufferevent_write_buffer");
	}
	else {
		if (bufferevent_get_enabled(from) & EV_READ) {
			redsocks_log_error(client, LOG_DEBUG, "backpressure: bufferevent_disable(%s, EV_READ)", bufname(client, from));
			if (bufferevent_disable(from, EV_READ) == -1)
				redsocks_log_errno(client, LOG_ERR, "bufferevent_disable");
		}
	}
}

static void redsocks_relay_writecb(redsocks_client *client, struct bufferevent *from, struct bufferevent *to)
{
	assert(from == client->client || from == client->relay);
	char from_eof = (from == client->client ? client->client_evshut : client->relay_evshut) & EV_READ;

	if (evbuffer_get_length(from->input) == 0 && from_eof) {
		redsocks_shutdown(client, to, SHUT_WR);
	}
	else if (evbuffer_get_length(to->output) < to->wm_write.high) {
		if (bufferevent_write_buffer(to, from->input) == -1)
			redsocks_log_errno(client, LOG_ERR, "bufferevent_write_buffer");
		if (!from_eof && !(bufferevent_get_enabled(from) & EV_READ)) {
			redsocks_log_error(client, LOG_DEBUG, "backpressure: bufferevent_enable(%s, EV_READ)", bufname(client, from));
			if (bufferevent_enable(from, EV_READ) == -1)
				redsocks_log_errno(client, LOG_ERR, "bufferevent_enable");
		}
	}
}

static void redsocks_relay_relayreadcb(struct bufferevent *from, void *_client)
{
	redsocks_client *client = _client;
	redsocks_touch_client(client);
	redsocks_relay_readcb(client, client->relay, client->client);
}

static void redsocks_relay_relaywritecb(struct bufferevent *to, void *_client)
{
	redsocks_client *client = _client;
	redsocks_touch_client(client);
	redsocks_relay_writecb(client, client->client, client->relay);
}

static void redsocks_relay_clientreadcb(struct bufferevent *from, void *_client)
{
	redsocks_client *client = _client;
	redsocks_touch_client(client);
	redsocks_relay_readcb(client, client->client, client->relay);
}

static void redsocks_relay_clientwritecb(struct bufferevent *to, void *_client)
{
	redsocks_client *client = _client;
	redsocks_touch_client(client);
	redsocks_relay_writecb(client, client->relay, client->client);
}

void redsocks_start_relay(redsocks_client *client)
{
	if (client->instance->relay_ss->fini)
		client->instance->relay_ss->fini(client);

	client->state = pump_active;

	int error = ((client->instance->config.use_splice) ? redsocks_start_splicepump : redsocks_start_bufferpump)(client);
	if (!error)
		redsocks_log_error(client, LOG_DEBUG, "data relaying started");
	else
		redsocks_drop_client(client);
}

static int redsocks_start_bufferpump(redsocks_client *client)
{
	// wm_write.high is respected by libevent-2.0.22 only for ssl and filters,
	// so it's implemented in redsocks callbacks.  wm_read.high works as expected.
	bufferevent_setwatermark(client->client, EV_READ|EV_WRITE, 0, REDSOCKS_RELAY_HALFBUFF);
	bufferevent_setwatermark(client->relay, EV_READ|EV_WRITE, 0, REDSOCKS_RELAY_HALFBUFF);

	client->client->readcb = redsocks_relay_clientreadcb;
	client->client->writecb = redsocks_relay_clientwritecb;
	client->relay->readcb = redsocks_relay_relayreadcb;
	client->relay->writecb = redsocks_relay_relaywritecb;

	int error = bufferevent_enable(client->client, (EV_READ|EV_WRITE) & ~(client->client_evshut));
	if (!error)
		error = bufferevent_enable(client->relay, (EV_READ|EV_WRITE) & ~(client->relay_evshut));
	if (error)
		redsocks_log_errno(client, LOG_ERR, "bufferevent_enable");
	return error;
}

static int pipeprio(redsocks_pump *pump, int fd)
{
	// client errors are logged with LOG_INFO, server errors with LOG_NOTICE
	return (fd == event_get_fd(&pump->client_read)) ? LOG_INFO : LOG_NOTICE;
}

static const char* pipename(redsocks_pump *pump, int fd)
{
	return (fd == event_get_fd(&pump->client_read)) ? "client" : "relay";
}

static void bufferevent_free_unused(struct bufferevent **p) {
	if (*p && !evbuffer_get_length((*p)->input) && !evbuffer_get_length((*p)->output)) {
		bufferevent_free(*p);
		*p = NULL;
	}
}

static bool would_block(int e) {
	return e == EAGAIN || e == EWOULDBLOCK;
}

typedef struct redsplice_write_ctx_t {
	// drain ebsrc[0], ebsrc[1], pisrc in that order
	struct evbuffer *ebsrc[2];
	splice_pipe *pisrc;
	struct event *evsrc;
	struct event *evdst;
	const evshut_t *shut_src;
	evshut_t *shut_dst;
} redsplice_write_ctx;

static void redsplice_write_cb(redsocks_pump *pump, redsplice_write_ctx *c, int out)
{
	bool has_data = false; // there is some pending data to be written
	bool can_write = true; // socket SEEMS TO BE writable

	// short write -- goto read/write management
	// write error -- drop client alltogether
	// full write  -- take next buffer
	// got EOF & no data -- relay EOF

	for (int i = 0; i < SIZEOF_ARRAY(c->ebsrc); ++i) {
		struct evbuffer *ebsrc = c->ebsrc[i];
		if (ebsrc) {
			const size_t avail = evbuffer_get_length(ebsrc);
			has_data = !!avail;
			if (avail) {
				const ssize_t sent = evbuffer_write(ebsrc, out);
				if (sent == -1) {
					if (would_block(errno)) { // short (zero) write
						can_write = false;
						goto decide;
					} else {
						redsocks_log_errno(&pump->c, pipeprio(pump, out), "evbuffer_write(to %s, %zu)", pipename(pump, out), avail);
						redsocks_drop_client(&pump->c);
						return;
					}
				} else if (avail == sent) {
					has_data = false; // unless stated otherwise
				} else { // short write
					can_write = false;
					goto decide;
				}
			}
		}
	}

	do {
		has_data = !!c->pisrc->size;
		const size_t avail = c->pisrc->size;
		if (avail) {
			const ssize_t sent = splice(c->pisrc->read, NULL, out, NULL, avail, SPLICE_F_MOVE|SPLICE_F_NONBLOCK);
			if (sent == -1) {
				if (would_block(errno)) { // short (zero) write
					can_write = false;
					goto decide;
				} else {
					redsocks_log_errno(&pump->c, pipeprio(pump, out), "splice(to %s)", pipename(pump, out));
					redsocks_drop_client(&pump->c);
					return;
				}
			} else {
				c->pisrc->size -= sent;
				if (avail == sent) {
					has_data = false;
				} else { // short write
					can_write = false;
					goto decide;
				}
			}
		}
	} while (0);

decide:
	if (!has_data && (*c->shut_src & EV_READ) && !(*c->shut_dst & EV_WRITE)) {
		if (shutdown(out, SHUT_WR) != 0) {
			redsocks_log_errno(&pump->c, LOG_ERR, "shutdown(%s, SHUT_WR)", pipename(pump, out));
		}
		*c->shut_dst |= EV_WRITE;
		can_write = false;
		assert(!c->pisrc->size);
		redsocks_close(c->pisrc->read);
		c->pisrc->read = -1;
		redsocks_close(c->pisrc->write);
		c->pisrc->write = -1;
		if (shut_both(&pump->c)) {
			redsocks_drop_client(&pump->c);
			return;
		}
	}

	assert(!(can_write && has_data)); // incomplete write to writable socket

	if (!can_write && has_data) {
		if (event_pending(c->evsrc, EV_READ, NULL))
			redsocks_log_error(&pump->c, LOG_DEBUG, "backpressure: event_del(%s_read)", pipename(pump, event_get_fd(c->evsrc)));
		redsocks_event_del(&pump->c, c->evsrc);
		redsocks_event_add(&pump->c, c->evdst);
	} else if (can_write && !has_data) {
		if (!event_pending(c->evsrc, EV_READ, NULL))
			redsocks_log_error(&pump->c, LOG_DEBUG, "backpressure: event_add(%s_read)", pipename(pump, event_get_fd(c->evsrc)));
		redsocks_event_add(&pump->c, c->evsrc);
		redsocks_event_del(&pump->c, c->evdst);
	} else if (!can_write && !has_data) { // something like EOF
		redsocks_event_del(&pump->c, c->evsrc);
		redsocks_event_del(&pump->c, c->evdst);
	}
}

typedef struct redsplice_read_ctx_t {
	splice_pipe *dst;
	struct event *evsrc;
	struct event *evdst;
	evshut_t *shut_src;
} redsplice_read_ctx;

static void redsplice_read_cb(redsocks_pump *pump, redsplice_read_ctx *c, int in)
{
	const size_t pipesize = 1048576; // some default value from fs.pipe-max-size
	const ssize_t got = splice(in, NULL, c->dst->write, NULL, pipesize, SPLICE_F_MOVE|SPLICE_F_NONBLOCK);
	if (got == -1) {
		if (would_block(errno)) {
			// there is data at `in', but pipe is full
			if (!event_pending(c->evsrc, EV_READ, NULL))
				redsocks_log_error(&pump->c, LOG_DEBUG, "backpressure: event_del(%s_read)", pipename(pump, event_get_fd(c->evsrc)));
			redsocks_event_del(&pump->c, c->evsrc);
		} else {
			redsocks_log_errno(&pump->c, pipeprio(pump, in), "splice(from %s)", pipename(pump, in));
			redsocks_drop_client(&pump->c);
		}
		return;
	}

	if (got == 0) { // got EOF
		if (shutdown(in, SHUT_RD) != 0) {
			if (errno != ENOTCONN) { // do not log common case for splice()
				redsocks_log_errno(&pump->c, LOG_DEBUG, "shutdown(%s, SHUT_RD) after EOF", pipename(pump, in));
			}
		}
		*c->shut_src |= EV_READ;
		redsocks_event_del(&pump->c, c->evsrc);
	} else {
		c->dst->size += got;
	}
	event_active(c->evdst, EV_WRITE, 0);
}

static void redsocks_touch_pump(redsocks_pump *pump)
{
	redsocks_touch_client(&pump->c);
	bufferevent_free_unused(&pump->c.client);
	bufferevent_free_unused(&pump->c.relay);
}

static void redsplice_relay_read(int fd, short what, void *_pump)
{
	redsocks_pump *pump = _pump;
	assert(fd == event_get_fd(&pump->relay_read) && (what & EV_READ));
	redsocks_touch_pump(pump);
	redsplice_read_ctx c = {
		.dst = &pump->reply,
		.evsrc = &pump->relay_read,
		.evdst = &pump->client_write,
		.shut_src = &pump->c.relay_evshut,
	};
	redsplice_read_cb(pump, &c, fd);
}

static void redsplice_client_read(int fd, short what, void *_pump)
{
	redsocks_pump *pump = _pump;
	assert(fd == event_get_fd(&pump->client_read) && (what & EV_READ));
	redsocks_touch_pump(pump);
	redsplice_read_ctx c = {
		.dst = &pump->request,
		.evsrc = &pump->client_read,
		.evdst = &pump->relay_write,
		.shut_src = &pump->c.client_evshut,
	};
	redsplice_read_cb(pump, &c, fd);
}

static void redsplice_relay_write(int fd, short what, void *_pump)
{
	redsocks_pump *pump = _pump;
	assert(fd == event_get_fd(&pump->relay_write) && (what & EV_WRITE));
	redsocks_touch_pump(pump);
	redsplice_write_ctx c = {
		.ebsrc = {
			pump->c.relay ? pump->c.relay->output : NULL,
			pump->c.client ? pump->c.client->input : NULL,
		},
		.pisrc = &pump->request,
		.evsrc = &pump->client_read,
		.evdst = &pump->relay_write,
		.shut_src = &pump->c.client_evshut,
		.shut_dst = &pump->c.relay_evshut,
	};
	redsplice_write_cb(pump, &c, fd);
}

static void redsplice_client_write(int fd, short what, void *_pump)
{
	redsocks_pump *pump = _pump;
	assert(fd == event_get_fd(&pump->client_write) && (what & EV_WRITE));
	redsocks_touch_pump(pump);
	redsplice_write_ctx c = {
		.ebsrc = {
			pump->c.client ? pump->c.client->output : NULL,
			pump->c.relay ? pump->c.relay->input : NULL,
		},
		.pisrc = &pump->reply,
		.evsrc = &pump->relay_read,
		.evdst = &pump->client_write,
		.shut_src = &pump->c.relay_evshut,
		.shut_dst = &pump->c.client_evshut,
	};
	redsplice_write_cb(pump, &c, fd);
}

static int redsocks_start_splicepump(redsocks_client *client)
{
	int error = bufferevent_disable(client->client, EV_READ|EV_WRITE);
	if (!error)
		error = bufferevent_disable(client->relay, EV_READ|EV_WRITE);
	if (error) {
		redsocks_log_errno(client, LOG_ERR, "bufferevent_disable");
		return error;
	}

	// going to steal this buffers to the socket
	evbuffer_unfreeze(client->client->input, 0);
	evbuffer_unfreeze(client->client->output, 1);
	evbuffer_unfreeze(client->relay->input, 0);
	evbuffer_unfreeze(client->relay->output, 1);

	redsocks_pump *pump = red_pump(client);
	if (!error)
		error = pipe2(&pump->request.read, O_NONBLOCK);
	if (!error)
		error = pipe2(&pump->reply.read, O_NONBLOCK);
	if (error) {
		redsocks_log_errno(client, LOG_ERR, "pipe2");
		return error;
	}

	struct event_base *base = NULL;
	const int relay_fd = bufferevent_getfd(client->relay);
	const int client_fd = bufferevent_getfd(client->client);
	if (!error)
		error = event_assign(&pump->client_read, base, client_fd, EV_READ|EV_PERSIST, redsplice_client_read, pump);
	if (!error)
		error = event_assign(&pump->client_write, base, client_fd, EV_WRITE|EV_PERSIST, redsplice_client_write, pump);
	if (!error)
		error = event_assign(&pump->relay_read, base, relay_fd, EV_READ|EV_PERSIST, redsplice_relay_read, pump);
	if (!error)
		error = event_assign(&pump->relay_write, base, relay_fd, EV_WRITE|EV_PERSIST, redsplice_relay_write, pump);
	if (error) {
		redsocks_log_errno(client, LOG_ERR, "event_assign");
		return error;
	}

	redsocks_bufferevent_dropfd(client, client->relay);
	redsocks_bufferevent_dropfd(client, client->client);

	// flush buffers (if any) and enable EV_READ callbacks
	event_active(&pump->client_write, EV_WRITE, 0);
	event_active(&pump->relay_write, EV_WRITE, 0);
	redsocks_event_add(&pump->c, &pump->client_read);
	redsocks_event_add(&pump->c, &pump->relay_read);

	return 0;
}

static bool has_loopback_destination(redsocks_client *client)
{
	const uint32_t net = ntohl(client->destaddr.sin_addr.s_addr) >> 24;
	return 0 == memcmp(&client->destaddr.sin_addr, &client->instance->config.relayaddr.sin_addr, sizeof(client->destaddr.sin_addr))
	    || net == 127 || net == 0;
}

void redsocks_drop_client(redsocks_client *client)
{
	if (shut_both(client)) {
		redsocks_log_error(client, LOG_INFO, "connection closed");
	} else {
		if (has_loopback_destination(client)) {
			static time_t last = 0;
			const time_t now = redsocks_time(NULL);
			if (now - last >= 3600) {
				// log this warning once an hour to save some debugging time, OTOH it may be valid traffic in some cases
				redsocks_log_error(client, LOG_NOTICE, "client tries to connect to the proxy using proxy! Usual proxy security policy is to drop alike connection");
				last = now;
			}
		}

		struct timeval now, idle;
		redsocks_gettimeofday(&now); // FIXME: use CLOCK_MONOTONIC
		timersub(&now, &client->last_event, &idle);
		redsocks_log_error(client, LOG_INFO, "dropping client (%s), relay (%s), idle %ld.%06lds",
			redsocks_event_str( (~client->client_evshut) & (EV_READ|EV_WRITE) ),
			redsocks_event_str( (~client->relay_evshut) & (EV_READ|EV_WRITE) ),
			idle.tv_sec, idle.tv_usec);
	}

	if (client->instance->relay_ss->fini)
		client->instance->relay_ss->fini(client);

	if (client->client)
		redsocks_bufferevent_free(client->client);

	if (client->relay)
		redsocks_bufferevent_free(client->relay);

	if (client->instance->config.use_splice) {
		redsocks_pump *pump = red_pump(client);

		if (pump->request.read != -1)
			redsocks_close(pump->request.read);
		if (pump->request.write != -1)
			redsocks_close(pump->request.write);
		if (pump->reply.read != -1)
			redsocks_close(pump->reply.read);
		if (pump->reply.write != -1)
			redsocks_close(pump->reply.write);

		// redsocks_close MAY log error if some of events was not properly initialized
		int fd = -1;
		if (event_initialized(&pump->client_read)) {
			fd = event_get_fd(&pump->client_read);
			redsocks_event_del(&pump->c, &pump->client_read);
		}
		if (event_initialized(&pump->client_write)) {
			redsocks_event_del(&pump->c, &pump->client_write);
		}
		if (fd != -1)
			redsocks_close(fd);

		fd = -1;
		if (event_initialized(&pump->relay_read)) {
			fd = event_get_fd(&pump->relay_read);
			redsocks_event_del(&pump->c, &pump->relay_read);
		}
		if (event_initialized(&pump->relay_write)) {
			redsocks_event_del(&pump->c, &pump->relay_write);
		}
		if (fd != -1) {
			redsocks_close(fd);
		}
	}
	redsocks_conn_list_del(client);
	free(client);
}

void redsocks_shutdown(redsocks_client *client, struct bufferevent *buffev, int how)
{
	short evhow = 0;
	const char *strev, *strhow = NULL, *strevhow = NULL;
	unsigned short *pevshut;

	assert(how == SHUT_RD || how == SHUT_WR || how == SHUT_RDWR);
	assert(buffev == client->client || buffev == client->relay);
	assert(event_get_fd(&buffev->ev_read) == event_get_fd(&buffev->ev_write));

	if (how == SHUT_RD) {
		strhow = "SHUT_RD";
		evhow = EV_READ;
		strevhow = "EV_READ";
	}
	else if (how == SHUT_WR) {
		strhow = "SHUT_WR";
		evhow = EV_WRITE;
		strevhow = "EV_WRITE";
	}
	else if (how == SHUT_RDWR) {
		strhow = "SHUT_RDWR";
		evhow = EV_READ|EV_WRITE;
		strevhow = "EV_READ|EV_WRITE";
	}

	assert(strhow && strevhow);

	strev = bufname(client, buffev);
	pevshut = buffev == client->client ? &client->client_evshut : &client->relay_evshut;

	// if EV_WRITE is already shut and we're going to shutdown read then
	// we're either going to abort data flow (bad behaviour) or confirm EOF
	// and in this case socket is already SHUT_RD'ed
	if ( !(how == SHUT_RD && (*pevshut & EV_WRITE)) ) {
		if (shutdown(event_get_fd(&buffev->ev_read), how) != 0)
			redsocks_log_errno(client, LOG_ERR, "shutdown(%s, %s)", strev, strhow);
	} else {
		redsocks_log_error(client, LOG_DEBUG, "ignored shutdown(%s, %s)", strev, strhow);
	}

	redsocks_log_error(client, LOG_DEBUG, "shutdown: bufferevent_disable(%s, %s)", strev, strevhow);
	if (bufferevent_disable(buffev, evhow) != 0)
		redsocks_log_errno(client, LOG_ERR, "bufferevent_disable(%s, %s)", strev, strevhow);

	*pevshut |= evhow;

	if (shut_both(client)) {
		redsocks_log_error(client, LOG_DEBUG, "both client and server disconnected");
		redsocks_drop_client(client);
	}
}

// I assume that -1 is invalid errno value
static int redsocks_socket_geterrno(redsocks_client *client, struct bufferevent *buffev)
{
	int pseudo_errno = red_socket_geterrno(buffev);
	if (pseudo_errno == -1) {
		redsocks_log_errno(client, LOG_ERR, "red_socket_geterrno");
		return -1;
	}
	return pseudo_errno;
}

static void redsocks_event_error(struct bufferevent *buffev, short what, void *_arg)
{
	redsocks_client *client = _arg;
	assert(buffev == client->relay || buffev == client->client);
	const int bakerrno = errno;

	redsocks_touch_client(client);

	if (what == (EVBUFFER_READ|EVBUFFER_EOF)) {
		struct bufferevent *antiev;
		if (buffev == client->relay)
			antiev = client->client;
		else
			antiev = client->relay;

		redsocks_shutdown(client, buffev, SHUT_RD);

		// If the client has already sent EOF and the pump is not active
		// (relay is activating), the code should not shutdown write-pipe.
		if (client->state == pump_active && antiev != NULL && evbuffer_get_length(antiev->output) == 0)
			redsocks_shutdown(client, antiev, SHUT_WR);
	}
	else {
		const int sockrrno = redsocks_socket_geterrno(client, buffev);
		const char *errsrc = "";
		if (sockrrno != -1 && sockrrno != 0) {
			errno = sockrrno;
			errsrc = "socket ";
		} else {
			errno = bakerrno;
		}
		redsocks_log_errno(client, bufprio(client, buffev), "%s %serror, code " event_fmt_str,
				bufname(client, buffev),
				errsrc,
				event_fmt(what));
		redsocks_drop_client(client);
	}
}

int sizes_equal(size_t a, size_t b)
{
	return a == b;
}

int sizes_greater_equal(size_t a, size_t b)
{
	return a >= b;
}

int redsocks_read_expected(redsocks_client *client, struct evbuffer *input, void *data, size_comparator comparator, size_t expected)
{
	size_t len = evbuffer_get_length(input);
	if (comparator(len, expected)) {
		int read = evbuffer_remove(input, data, expected);
		UNUSED(read);
		assert(read == expected);
		return 0;
	}
	else {
		redsocks_log_error(client, LOG_NOTICE, "Can't get expected amount of data");
		redsocks_drop_client(client);
		return -1;
	}
}

struct evbuffer *mkevbuffer(void *data, size_t len)
{
	struct evbuffer *buff = NULL, *retval = NULL;

	buff = evbuffer_new();
	if (!buff) {
		log_errno(LOG_ERR, "evbuffer_new");
		goto fail;
	}

	if (evbuffer_add(buff, data, len) < 0) {
		log_errno(LOG_ERR, "evbuffer_add");
		goto fail;
	}

	retval = buff;
	buff = NULL;

fail:
	if (buff)
		evbuffer_free(buff);
	return retval;
}

int redsocks_write_helper_ex(
	struct bufferevent *buffev, redsocks_client *client,
	redsocks_message_maker mkmessage, int state, size_t wm_low, size_t wm_high)
{
	assert(client);
	return redsocks_write_helper_ex_plain(buffev, client, (redsocks_message_maker_plain)mkmessage,
	                                      client, state, wm_low, wm_high);
}

int redsocks_write_helper_ex_plain(
	struct bufferevent *buffev, redsocks_client *client,
	redsocks_message_maker_plain mkmessage, void *p, int state, size_t wm_low, size_t wm_high)
{
	int len;
	struct evbuffer *buff = NULL;
	int drop = 1;

	if (mkmessage) {
		buff = mkmessage(p);
		if (!buff)
			goto fail;

		assert(!client || buffev == client->relay);
		len = bufferevent_write_buffer(buffev, buff);
		if (len < 0) {
			if (client)
				redsocks_log_errno(client, LOG_ERR, "bufferevent_write_buffer");
			else
				log_errno(LOG_ERR, "bufferevent_write_buffer");
			goto fail;
		}
	}

	if (client)
		client->state = state;
	bufferevent_setwatermark(buffev, EV_READ, wm_low, wm_high);
	bufferevent_enable(buffev, EV_READ);
	drop = 0;

fail:
	if (buff)
		evbuffer_free(buff);
	if (drop && client)
		redsocks_drop_client(client);
	return drop ? -1 : 0;
}

int redsocks_write_helper(
	struct bufferevent *buffev, redsocks_client *client,
	redsocks_message_maker mkmessage, int state, size_t wm_only)
{
	assert(client);
	return redsocks_write_helper_ex(buffev, client, mkmessage, state, wm_only, wm_only);
}

static void redsocks_relay_connected(struct bufferevent *buffev, void *_arg)
{
	redsocks_client *client = _arg;

	assert(buffev == client->relay);

	redsocks_touch_client(client);

	if (!red_is_socket_connected_ok(buffev)) {
		redsocks_log_errno(client, LOG_NOTICE, "red_is_socket_connected_ok");
		goto fail;
	}

	client->relay->readcb = client->instance->relay_ss->readcb;
	client->relay->writecb = client->instance->relay_ss->writecb;
	client->relay->writecb(buffev, _arg);
	return;

fail:
	redsocks_drop_client(client);
}

void redsocks_connect_relay(redsocks_client *client)
{
	client->relay = red_connect_relay(&client->instance->config.relayaddr,
			                          redsocks_relay_connected, redsocks_event_error, client);
	if (!client->relay) {
		redsocks_log_errno(client, LOG_ERR, "red_connect_relay");
		redsocks_drop_client(client);
	}
}

static struct timeval drop_idle_connections()
{
	assert(connpres_idle_timeout() > 0);
	struct timeval now, zero, max_idle, best_next;
	gettimeofday(&now, NULL); // FIXME: use CLOCK_MONOTONIC
	timerclear(&zero);
	timerclear(&max_idle);
	max_idle.tv_sec = connpres_idle_timeout();
	best_next = max_idle;

	redsocks_instance *instance;
	list_for_each_entry(instance, &instances, list) {
		redsocks_client *tmp, *client;
		list_for_each_entry_safe(client, tmp, &instance->clients, list) {
			struct timeval idle;
			timersub(&now, &client->last_event, &idle);
			if (timercmp(&idle, &zero, <=) || timercmp(&max_idle, &idle, <=)) {
				redsocks_drop_client(client);
				best_next = zero;
			} else {
				struct timeval delay;
				timersub(&max_idle, &idle, &delay);
				if (timercmp(&delay, &best_next, <)) {
					best_next = delay;
				}
			}
		}
	}
	return best_next;
}

static bool conn_pressure_ongoing()
{
	if (redsocks_conn >= redsocks_conn_max())
		return true;
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd == -1)
		return true;
	close(fd);
	return false;
}

static void conn_pressure()
{
	struct timeval next;
	timerclear(&next);

	if (connpres_idle_timeout()) {
		next = drop_idle_connections();
		if (!timerisset(&next)) {
			log_error(LOG_WARNING, "dropped connections idle for %d+ seconds", connpres_idle_timeout());
			return; // pressure solved
		}
	}

	accept_backoff_ms = (accept_backoff_ms << 1) + 1;
	clamp_value(accept_backoff_ms, 1, max_accept_backoff_ms());
	uint32_t delay = (red_randui32() % accept_backoff_ms) + 1;
	struct timeval tvdelay = { delay / 1000, (delay % 1000) * 1000 };

	if (timerisset(&next) && timercmp(&next, &tvdelay, <) ) {
		tvdelay = next;
	}

	log_error(LOG_WARNING, "accept: backing off for %ld.%06lds", tvdelay.tv_sec, tvdelay.tv_usec);

	if (event_add(&accept_backoff_ev, &tvdelay) != 0)
		log_errno(LOG_ERR, "event_add");

	redsocks_instance *self = NULL;
	list_for_each_entry(self, &instances, list) {
		if (event_del(&self->listener) != 0)
			log_errno(LOG_ERR, "event_del");
	}
}

// if there are no idle connections delay to the nearest one is returned
static void accept_enable()
{
	redsocks_instance *self = NULL;
	list_for_each_entry(self, &instances, list) {
		if (event_add(&self->listener, NULL) != 0)
			log_errno(LOG_ERR, "event_add");
	}
}

static void conn_pressure_lowered()
{
	if (redsocks_conn >= redsocks_conn_max())
		return; // lowered... not so much!

	if (event_pending(&accept_backoff_ev, EV_TIMEOUT, NULL)) {
		if (event_del(&accept_backoff_ev) != 0)
			log_errno(LOG_ERR, "event_del");
		accept_enable();
	}
}

static void redsocks_accept_backoff(int fd, short what, void *_null)
{
	if (conn_pressure_ongoing()) {
		conn_pressure(); // rearm timeout
	} else {
		accept_enable(); // `accept_backoff_ev` is not pending now
	}
}

void redsocks_close_internal(int fd, const char* file, int line, const char *func)
{
	if (close(fd) == 0) {
		conn_pressure_lowered();
	}
	else {
		const int do_errno = 1;
		_log_write(file, line, func, do_errno, LOG_WARNING, "close");
	}
}

void redsocks_event_add_internal(redsocks_client *client, struct event *ev, const char *file, int line, const char *func)
{
	if (event_add(ev, NULL) != 0) {
		const int do_errno = 1;
		redsocks_log_write_plain(file, line, func, do_errno, &(client)->clientaddr, &(client)->destaddr, LOG_WARNING, "event_add");
	}
}

void redsocks_event_del_internal(redsocks_client *client, struct event *ev, const char *file, int line, const char *func)
{
	if (event_del(ev) != 0) {
		const int do_errno = 1;
		redsocks_log_write_plain(file, line, func, do_errno, &(client)->clientaddr, &(client)->destaddr, LOG_WARNING, "event_del");
	}
}

void redsocks_bufferevent_dropfd_internal(redsocks_client *client, struct bufferevent *ev, const char *file, int line, const char *func)
{
	if (bufferevent_setfd(ev, -1) != 0) {
		const int do_errno = 1;
		redsocks_log_write_plain(file, line, func, do_errno, &(client)->clientaddr, &(client)->destaddr, LOG_WARNING, "bufferevent_setfd");
	}
}

void redsocks_bufferevent_free(struct bufferevent *buffev)
{
	int fd = bufferevent_getfd(buffev);
	if (bufferevent_setfd(buffev, -1)) { // to avoid EBADFD warnings from epoll
		log_errno(LOG_WARNING, "bufferevent_setfd");
	}
	bufferevent_free(buffev);
	if (fd != -1)
		redsocks_close(fd);
}

static void redsocks_conn_list_add(redsocks_instance *self, redsocks_client *client)
{
	assert(list_empty(&client->list));
	assert(redsocks_conn < redsocks_conn_max());
	list_add(&client->list, &self->clients);
	redsocks_conn++;
	if (redsocks_conn >= redsocks_conn_max()) {
		log_error(LOG_WARNING, "reached redsocks_conn_max limit, %d connections", redsocks_conn);
		conn_pressure();
	}
}

static void redsocks_conn_list_del(redsocks_client *client)
{
	if (!list_empty(&client->list)) {
		redsocks_conn--;
		list_del(&client->list);
	}
	conn_pressure_lowered();
}

static void redsocks_accept_client(int fd, short what, void *_arg)
{
	redsocks_instance *self = _arg;
	redsocks_client   *client = NULL;
	struct sockaddr_in clientaddr;
	struct sockaddr_in myaddr;
	struct sockaddr_in destaddr;
	socklen_t          addrlen = sizeof(clientaddr);
	int client_fd = -1;
	int error;

	assert(redsocks_conn < redsocks_conn_max());

	// working with client_fd
	client_fd = accept(fd, (struct sockaddr*)&clientaddr, &addrlen);
	if (client_fd == -1) {
		const int e = errno;
		log_errno(LOG_WARNING, "accept");
		/* Different systems use different `errno` value to signal different
		 * `lack of file descriptors` conditions. Here are most of them.  */
		if (e == ENFILE || e == EMFILE || e == ENOBUFS || e == ENOMEM) {
			conn_pressure();
		}
		goto fail;
	}
	accept_backoff_ms = 0;

	// socket is really bound now (it could be bound to 0.0.0.0)
	addrlen = sizeof(myaddr);
	error = getsockname(client_fd, (struct sockaddr*)&myaddr, &addrlen);
	if (error) {
		log_errno(LOG_WARNING, "getsockname");
		goto fail;
	}

	error = getdestaddr(client_fd, &clientaddr, &myaddr, &destaddr);
	if (error) {
		goto fail;
	}

	error = fcntl_nonblock(client_fd);
	if (error) {
		log_errno(LOG_ERR, "fcntl");
		goto fail;
	}

	if (apply_tcp_keepalive(client_fd))
		goto fail;

	// everything seems to be ok, let's allocate some memory
	client = calloc(1, sizeof_client(self));
	if (!client) {
		log_errno(LOG_ERR, "calloc");
		goto fail;
	}
	client->instance = self;
	if (client->instance->config.use_splice) {
		redsocks_pump *pump = red_pump(client);
		pump->request.read = -1;
		pump->request.write = -1;
		pump->reply.read = -1;
		pump->reply.write = -1;
	}
	memcpy(&client->clientaddr, &clientaddr, sizeof(clientaddr));
	memcpy(&client->destaddr, &destaddr, sizeof(destaddr));
	INIT_LIST_HEAD(&client->list);
	self->relay_ss->init(client);

	if (redsocks_gettimeofday(&client->first_event) != 0)
		goto fail;

	redsocks_touch_client(client);

	client->client = bufferevent_new(client_fd, NULL, NULL, redsocks_event_error, client);
	if (!client->client) {
		log_errno(LOG_ERR, "bufferevent_new");
		goto fail;
	}
	client_fd = -1;

	redsocks_conn_list_add(self, client);

	// enable reading to handle EOF from client
	if (bufferevent_enable(client->client, EV_READ) != 0) {
		redsocks_log_errno(client, LOG_ERR, "bufferevent_enable");
		goto fail;
	}

	redsocks_log_error(client, LOG_INFO, "accepted");

	if (self->relay_ss->connect_relay)
		self->relay_ss->connect_relay(client);
	else
		redsocks_connect_relay(client);

	return;

fail:
	if (client) {
		redsocks_drop_client(client);
	}
	if (client_fd != -1)
		redsocks_close(client_fd);
}

static const char *redsocks_evshut_str(unsigned short evshut)
{
	return
		evshut == EV_READ ? "SHUT_RD" :
		evshut == EV_WRITE ? "SHUT_WR" :
		evshut == (EV_READ|EV_WRITE) ? "SHUT_RDWR" :
		evshut == 0 ? "" :
		"???";
}

static const char *redsocks_event_str(unsigned short what)
{
	return
		what == EV_READ ? "R/-" :
		what == EV_WRITE ? "-/W" :
		what == (EV_READ|EV_WRITE) ? "R/W" :
		what == 0 ? "-/-" :
		"???";
}

static uint32_t redsocks_debug_dump_instance(redsocks_instance *instance, struct timeval *now)
{
	redsocks_client *client = NULL;
	uint32_t conn = 0;
	char bindaddr_str[RED_INET_ADDRSTRLEN];

	log_error(LOG_NOTICE, "Dumping client list for %s at %s:", instance->config.type, red_inet_ntop(&instance->config.bindaddr, bindaddr_str, sizeof(bindaddr_str)));
	list_for_each_entry(client, &instance->clients, list) {
		conn++;
		const char *s_client_evshut = redsocks_evshut_str(client->client_evshut);
		const char *s_relay_evshut = redsocks_evshut_str(client->relay_evshut);
		struct timeval age, idle;
		timersub(now, &client->first_event, &age);
		timersub(now, &client->last_event, &idle);

		if (!instance->config.use_splice) {
			redsocks_log_error(client, LOG_NOTICE, "client: %i (%s)%s%s, relay: %i (%s)%s%s, age: %ld.%06ld sec, idle: %ld.%06ld sec.",
				client->client ? bufferevent_getfd(client->client) : -1,
				client->client ? redsocks_event_str(client->client->enabled) : "NULL",
				s_client_evshut[0] ? " " : "",
				s_client_evshut,
				client->relay ? bufferevent_getfd(client->relay) : -1,
				client->relay ? redsocks_event_str(client->relay->enabled) : "NULL",
				s_relay_evshut[0] ? " " : "",
				s_relay_evshut,
				age.tv_sec, age.tv_usec,
				idle.tv_sec, idle.tv_usec);
		} else {
			redsocks_pump *pump = red_pump(client);

			redsocks_log_error(client, LOG_NOTICE, "client: buf %i (%s) splice %i (%s/%s) pipe[%d, %d]=%zu%s%s, relay: buf %i (%s) splice %i (%s/%s) pipe[%d, %d]=%zu%s%s, age: %ld.%06ld sec, idle: %ld.%06ld sec.",
				client->client ? bufferevent_getfd(client->client) : -1,
				client->client ? redsocks_event_str(client->client->enabled) : "NULL",
				event_get_fd(&pump->client_read),
				event_pending(&pump->client_read, EV_READ, NULL) ? "R" : "-",
				event_pending(&pump->client_write, EV_WRITE, NULL) ? "W" : "-",
				pump->request.read,
				pump->request.write,
				pump->request.size,
				s_client_evshut[0] ? " " : "",
				s_client_evshut,

				client->relay ? bufferevent_getfd(client->relay) : -1,
				client->relay ? redsocks_event_str(client->relay->enabled) : "NULL",
				event_get_fd(&pump->relay_read),
				event_pending(&pump->relay_read, EV_READ, NULL) ? "R" : "-",
				event_pending(&pump->relay_write, EV_WRITE, NULL) ? "W" : "-",
				pump->reply.read,
				pump->reply.write,
				pump->reply.size,
				s_relay_evshut[0] ? " " : "",
				s_relay_evshut,

				age.tv_sec, age.tv_usec,
				idle.tv_sec, idle.tv_usec);
		}
	}
	log_error(LOG_NOTICE, "End of client list. %d clients.", conn);
	return conn;
}

static void redsocks_debug_dump(int sig, short what, void *_arg)
{
	redsocks_instance *instance = NULL;
	struct timeval now;
	redsocks_gettimeofday(&now);
	uint32_t conn = 0;

	list_for_each_entry(instance, &instances, list)
		conn += redsocks_debug_dump_instance(instance, &now);
	assert(conn == redsocks_conn);
}

bool redsocks_has_splice_instance()
{
	// only i->config is initialised at the moment
	redsocks_instance *i = NULL;
	list_for_each_entry(i, &instances, list) {
		if (i->config.use_splice)
			return true;
	}
	return false;
}

static void redsocks_fini_instance(redsocks_instance *instance);

static int redsocks_init_instance(redsocks_instance *instance)
{
	/* FIXME: redsocks_fini_instance is called in case of failure, this
	 *        function will remove instance from instances list - result
	 *        looks ugly.
	 */
	int error;
	int on = 1;
	int fd = -1;

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd == -1) {
		log_errno(LOG_ERR, "socket");
		goto fail;
	}

	error = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	if (error) {
		log_errno(LOG_ERR, "setsockopt");
		goto fail;
	}

	error = bind(fd, (struct sockaddr*)&instance->config.bindaddr, sizeof(instance->config.bindaddr));
	if (error) {
		log_errno(LOG_ERR, "bind");
		goto fail;
	}

	error = fcntl_nonblock(fd);
	if (error) {
		log_errno(LOG_ERR, "fcntl");
		goto fail;
	}

	error = listen(fd, instance->config.listenq);
	if (error) {
		log_errno(LOG_ERR, "listen");
		goto fail;
	}

	event_set(&instance->listener, fd, EV_READ | EV_PERSIST, redsocks_accept_client, instance);
	fd = -1;

	error = event_add(&instance->listener, NULL);
	if (error) {
		log_errno(LOG_ERR, "event_add");
		goto fail;
	}

	if (instance->relay_ss->instance_init)
		instance->relay_ss->instance_init(instance);

	return 0;

fail:
	redsocks_fini_instance(instance);

	if (fd != -1) {
		redsocks_close(fd);
	}

	return -1;
}

/* Drops instance completely, freeing its memory and removing from
 * instances list.
 */
static void redsocks_fini_instance(redsocks_instance *instance) {
	if (!list_empty(&instance->clients)) {
		redsocks_client *tmp, *client = NULL;

		log_error(LOG_WARNING, "There are connected clients during shutdown! Disconnecting them.");
		list_for_each_entry_safe(client, tmp, &instance->clients, list) {
			redsocks_drop_client(client);
		}
	}

	if (instance->relay_ss->instance_fini)
		instance->relay_ss->instance_fini(instance);

	if (event_initialized(&instance->listener)) {
		if (event_del(&instance->listener) != 0)
			log_errno(LOG_WARNING, "event_del");
		redsocks_close(event_get_fd(&instance->listener));
		memset(&instance->listener, 0, sizeof(instance->listener));
	}

	list_del(&instance->list);

	free(instance->config.type);
	free(instance->config.login);
	free(instance->config.password);

	memset(instance, 0, sizeof(*instance));
	free(instance);
}

static int redsocks_fini();

static struct event debug_dumper;

static int redsocks_init() {
	struct sigaction sa = { }, sa_old = { };
	redsocks_instance *tmp, *instance = NULL;

	redsocks_conn = 0;

	sa.sa_handler = SIG_IGN;
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGPIPE, &sa, &sa_old) == -1) {
		log_errno(LOG_ERR, "sigaction");
		return -1;
	}

	signal_set(&debug_dumper, SIGUSR1, redsocks_debug_dump, NULL);
	if (signal_add(&debug_dumper, NULL) != 0) {
		log_errno(LOG_ERR, "signal_add");
		goto fail;
	}

	event_set(&accept_backoff_ev, -1, 0, redsocks_accept_backoff, NULL);

	list_for_each_entry_safe(instance, tmp, &instances, list) {
		if (redsocks_init_instance(instance) != 0)
			goto fail;
	}

	return 0;

fail:
	// that was the first resource allocation, it return's on failure, not goto-fail's
	sigaction(SIGPIPE, &sa_old, NULL);

	redsocks_fini();

	return -1;
}

static int redsocks_fini()
{
	redsocks_instance *tmp, *instance = NULL;

	list_for_each_entry_safe(instance, tmp, &instances, list)
		redsocks_fini_instance(instance);

	assert(redsocks_conn == 0);

	if (signal_initialized(&debug_dumper)) {
		if (signal_del(&debug_dumper) != 0)
			log_errno(LOG_WARNING, "signal_del");
		memset(&debug_dumper, 0, sizeof(debug_dumper));
	}

	return 0;
}

app_subsys redsocks_subsys =
{
	.init = redsocks_init,
	.fini = redsocks_fini,
	.conf_section = &redsocks_conf_section,
};



/* vim:set tabstop=4 softtabstop=4 shiftwidth=4: */
/* vim:set foldmethod=marker foldlevel=32 foldmarker={,}: */
