#ifndef MODULE_H_INCLUDED
#define MODULE_H_INCLUDED

#include <ltdl.h>
#include <reactor.h>
#include <jansson.h>
#include <flow.h>

struct graph;

typedef flow_module_table  module_table;
typedef struct module      module;
typedef struct modules     modules;

struct module
{
  const char    *name;
  lt_dlhandle    handle;
  module_table  *table;
  void          *state;
};

struct modules
{
  list           modules;
};

void    modules_construct(modules *);
void    modules_search(modules *, const char *);
module *modules_load(modules *, struct graph *, const char *);
module *modules_lookup(modules *, const char *);
module *modules_match(modules *, struct graph *, const char *);
void    modules_destruct(modules *);

void   *module_load(module *, struct graph *);
void   *module_create(module *, void *, json_t *);
void    module_receive(module *, void *, void *);
void    module_destroy(module *, void *);
void    module_unload(module *, void *);

#endif /* MODULE_H_INCLUDED */
