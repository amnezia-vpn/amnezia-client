#ifndef PARSER_H_THU_JAN_11_04_49_38_2007
#define PARSER_H_THU_JAN_11_04_49_38_2007

#include <stdio.h>
#include <stdbool.h>

enum disclose_src_e {
	DISCLOSE_NONE,
	DISCLOSE_X_FORWARDED_FOR,
	DISCLOSE_FORWARDED_IP,
	DISCLOSE_FORWARDED_IPPORT,
};

enum on_proxy_fail_e {
	ONFAIL_CLOSE,
	ONFAIL_FORWARD_HTTP_ERR,
};

typedef enum {
	pt_bool,      // "bool" from stdbool.h, not "_Bool" or anything else
	pt_pchar,
	pt_uint16,
	pt_uint32,
	pt_in_addr,
	pt_in_addr2,  // inaddr[0] = net, inaddr[1] = netmask
	pt_disclose_src,
	pt_on_proxy_fail,
	pt_obsolete,
	pt_redsocks_max_accept_backoff,
} parser_type;

typedef struct parser_entry_t {
	const char    *key;
	parser_type    type;
	void          *addr;
} parser_entry;


typedef struct parser_context_t parser_context;


typedef struct parser_section_t parser_section;
typedef int  (*parser_section_onenter)(parser_section *section);
typedef int  (*parser_section_onexit)(parser_section *section);

struct parser_section_t {
	parser_section         *next;
	parser_context         *context;
	const char             *name;
	parser_section_onenter  onenter; // is called on entry to section
	parser_section_onexit   onexit;  // is called on exit from section
	parser_entry           *entries;
	void                   *data;
};



parser_context* parser_start(FILE *fd);
void parser_add_section(parser_context *context, parser_section *section);
int parser_run(parser_context *context);
void parser_error(parser_context *context, const char *fmt, ...)
#if defined(__GNUC__)
	__attribute__ (( format (printf, 2, 3) ))
#endif
;
void parser_stop(parser_context *context);

/* vim:set tabstop=4 softtabstop=4 shiftwidth=4: */
/* vim:set foldmethod=marker foldlevel=32 foldmarker={,}: */
#endif /* PARSER_H_THU_JAN_11_04_49_38_2007 */
