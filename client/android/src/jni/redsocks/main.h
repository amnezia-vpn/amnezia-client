#ifndef MAIN_H_TUE_JAN_23_15_38_25_2007
#define MAIN_H_TUE_JAN_23_15_38_25_2007

#include "parser.h"

struct event_base;

typedef struct app_subsys_t {
	int (*init)(struct event_base*);
	int (*fini)();
	parser_section* conf_section;
} app_subsys;

#define SIZEOF_ARRAY(arr)        (sizeof(arr) / sizeof(arr[0]))
#define FOREACH(ptr, array)      for (ptr = array; ptr < array + SIZEOF_ARRAY(array); ptr++)
#define FOREACH_REV(ptr, array)  for (ptr = array + SIZEOF_ARRAY(array) - 1; ptr >= array; ptr--)


/* vim:set tabstop=4 softtabstop=4 shiftwidth=4: */
/* vim:set foldmethod=marker foldlevel=32 foldmarker={,}: */
#endif /* MAIN_H_TUE_JAN_23_15_38_25_2007 */
