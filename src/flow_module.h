#ifndef FLOW_MODULE_H_INCLUDED
#define FLOW_MODULE_H_INCLUDED

#include <ltdl.h>
#include <jansson.h>
#include <dynamic.h>

#include "flow_message.h"

typedef struct flow_module_table flow_module_table;
typedef struct flow_module       flow_module;
typedef list                     flow_modules;

struct flow_module_table
{
  void *(*load)(void);
  void  (*unload)(void *);
  void *(*create)(void *, json_t *);
  void  (*destroy)(void *);
  void  (*receive)(void *, void *);
};

struct flow_module
{
  char              *name;
  lt_dlhandle        handle;
  flow_module_table *table;
  void              *state;
};

void         flow_modules_construct(flow_modules *);
void         flow_modules_search(const char *);
void         flow_modules_load(flow *, const char *);
void         flow_modules_register(flow *, const char *);
flow_module *flow_modules_lookup(flow_modules *, const char *);
flow_module *flow_modules_match_prefix(flow_modules *, const char *);

void         flow_modules_destruct(flow *);

void         flow_module_construct(flow *, flow_module *, const char *);
void         flow_module_construct_static(flow *, flow_module *, const char *);
void         flow_module_destruct(flow_module *);
void         flow_module_load(flow_module *);
void         flow_module_unload(flow_module *);
void        *flow_module_create(flow_module *, void *, json_t *);
void         flow_module_destroy(flow_module *, void *);
void         flow_module_receive(flow_module *, void *, void *);

#endif /* FLOW_MODULE_H_INCLUDED */
