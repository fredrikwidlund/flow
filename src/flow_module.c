#define _GNU_SOURCE

#include <stdio.h>
#include <assert.h>
#include <err.h>
#include <string.h>
#include <dlfcn.h>
#include <ltdl.h>

#include "flow.h"

/****************/
/* flow modules */
/****************/

static _Atomic size_t flow_modules_ref = 0;

void flow_modules_construct(flow_modules *modules)
{
  size_t ref = __sync_add_and_fetch(&flow_modules_ref, 1);

  if (ref == 1)
  {
    assert(lt_dlinit() == 0);
    assert(lt_dlsetsearchpath("/usr/lib/flow") == 0);
  }

  list_construct(modules);
}

void flow_modules_search(const char *path)
{
  assert(lt_dlinsertsearchdir(lt_dlgetsearchpath(), path) == 0);
}

flow_module *flow_modules_lookup(flow_modules *modules, const char *name)
{
  flow_module *module;

  if (name)
    list_foreach(modules, module) if (strcmp(module->name, name) == 0) return module;
  return NULL;
}

flow_module *flow_modules_match_prefix(flow_modules *modules, const char *name)
{
  flow_module *module, *module_matched = NULL;
  size_t l, match_length = 0;

  if (!name)
    return NULL;

  list_foreach(modules, module)
  {
    l = strlen(module->name);
    if (l > match_length && strncmp(module->name, name, l) == 0)
    {
      module_matched = module;
      match_length = l;
    }
  }

  return module_matched;
}

void flow_modules_load(flow *flow, const char *name)
{
  flow_modules *modules = &flow->modules;

  if (!flow_modules_lookup(modules, name))
    flow_module_construct(flow, list_push_back(modules, NULL, sizeof(flow_module)), name);
}

void flow_modules_register(flow *flow, const char *name)
{
  flow_modules *modules = &flow->modules;

  if (!flow_modules_lookup(modules, name))
    flow_module_construct_static(flow, list_push_back(modules, NULL, sizeof(flow_module)), name);
}

void flow_modules_destruct(flow *flow)
{
  flow_modules *modules = &flow->modules;
  flow_module *module;
  size_t ref = __sync_sub_and_fetch(&flow_modules_ref, 1);

  list_foreach(modules, module)
  {
    flow_log(flow, FLOW_DEBUG, "removing module %s", module->name);
    flow_module_unload(module);
    flow_module_destruct(module);
  }

  list_destruct(modules, NULL);

  if (ref == 0)
  {
    assert(lt_dlexit() == 0);
  }
}

/***************/
/* flow module */
/***************/

void flow_module_construct_static(flow *flow, flow_module *module, const char *name)
{
  char *symbol_name;

  module->name = strdup(name);
  asprintf(&symbol_name, "%s_module_table", name);
  module->table = dlsym(NULL, symbol_name);
  free(symbol_name);
  if (!module->table)
    flow_log(flow, FLOW_CRITICAL, "unable to locate module table in module %s", name);

  flow_module_load(module, flow);
}

void flow_module_construct(flow *flow, flow_module *module, const char *name)
{
  lt_dladvise advise;
  char *symbol_name;

  module->name = strdup(name);
  lt_dladvise_init(&advise);
  lt_dladvise_ext(&advise);
  lt_dladvise_global(&advise);

  module->handle = lt_dlopenadvise(module->name, advise);
  if (!module->handle)
    flow_log(flow, FLOW_CRITICAL, "unable to load module %s, %s", name, lt_dlerror());

  asprintf(&symbol_name, "%s_module_table", name);
  module->table = lt_dlsym(module->handle, symbol_name);
  free(symbol_name);
  if (!module->table)
    flow_log(flow, FLOW_CRITICAL, "unable to locate module table in module %s", name);
  lt_dladvise_destroy(&advise);

  flow_module_load(module, flow);
}

void flow_module_destruct(flow_module *module)
{
  if (module->handle)
    lt_dlclose(module->handle);
  free(module->name);
}

void flow_module_load(flow_module *module, flow *flow)
{
  if (module->table->load)
    module->state = module->table->load(flow);
}

void flow_module_unload(flow_module *module)
{
  if (module->table->unload)
    module->table->unload(module->state);
}

void *flow_module_create(flow_module *module, void *node, json_t *metadata)
{
  return module->table->create ? module->table->create(node, metadata) : NULL;
}

void flow_module_destroy(flow_module *module, void *state)
{
  if (module->table->destroy)
    module->table->destroy(state);
}

void flow_module_receive(flow_module *module, void *state, void *message)
{
  if (module->table->receive)
    module->table->receive(state, message);
}
