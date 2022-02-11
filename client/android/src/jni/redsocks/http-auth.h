#ifndef HTTP_AUTH_H
#define HTTP_AUTH_H

#include "redsocks.h"

typedef struct http_auth_t {
	char *last_auth_query;
	int last_auth_count;
} http_auth;

static inline http_auth* red_http_auth(redsocks_instance *i)
{
	return (http_auth*)(i + 1);
}

/*
 * Create the authentication header contents for the `Basic' scheme.
 * This is done by encoding the string "USER:PASS" to base64 and
 * prepending the string "Basic " in front of it.
 *
 */

char* basic_authentication_encode(const char *user, const char *passwd);

/*
 * Create the authentication header contents for the 'Digest' scheme.
 * only md5 method is available, see RFC 2617 for detail.
 *
 */
char* digest_authentication_encode(const char *line, const char *user, const char *passwd, 
		const char *method, const char *path, int count, const char *cnonce);

char *http_auth_request_header(struct evbuffer *src, struct evbuffer *tee);

#endif /* HTTP_AUTH_H */
