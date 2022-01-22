#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "modules.h"
#include "log.h"

static _Atomic size_t module_ref = 0;

static void module_destruct(void *arg)
{
  module *module = arg;

  module_unload(module, module->state);
  if (module->handle)
    lt_dlclose(module->handle);
}

void modules_construct(modules *pool)
{
  size_t ref = __sync_add_and_fetch(&module_ref, 1);

  if (ref == 1)
  {
    assert(lt_dlinit() == 0);
    assert(lt_dlsetsearchpath("/usr/lib/flow") == 0);
  }

  list_construct(&pool->modules);
}

void modules_search(modules *pool, const char *path)
{
  assert(lt_dlinsertsearchdir(lt_dlgetsearchpath(), path) == 0);
}

module *modules_lookup(modules *pool, const char *name)
{
  module *module;

  if (!name)
    return NULL;
  list_foreach(&pool->modules, module)
  {
    if (strcmp(module->name, name) == 0)
      return module;
  }
  return NULL;
}

module *modules_match(modules *pool, struct graph *graph, const char *name)
{
  module *module, *match = NULL;
  size_t l, match_length = 0;

  if (!name)
    return NULL;

  list_foreach(&pool->modules, module)
  {
    l = strlen(module->name);
    if (l > match_length && strncmp(module->name, name, l) == 0)
    {
      match = module;
      match_length = l;
    }
  }

  return match ? match : modules_load(pool, graph, name);
}

module *modules_load(modules *pool, struct graph *graph, const char *name)
{
  module *module;
  lt_dladvise advise;
  string table_name;

  if (!name)
    return NULL;
  module = modules_lookup(pool, name);
  if (module)
    return module;
  module = list_push_back(&pool->modules, NULL, sizeof *module);
  module->name = name;

  lt_dladvise_init(&advise);
  lt_dladvise_ext(&advise);
  lt_dladvise_global(&advise);

  module->handle = lt_dlopenadvise(module->name, advise);
  if (!module->handle)
    log_critical(graph, "unable to load module %s, %s", name, lt_dlerror());

  table_name = string_append_data(string_null(), data_string(module->name));
  table_name = string_append_data(table_name, data_string("_module_table"));
  module->table = lt_dlsym(module->handle, table_name);
  if (!module->table)
    log_critical(graph, "unable to locate module table in %s", name);
  string_free(table_name);

  lt_dladvise_destroy(&advise);
  module->state = module_load(module, graph);
  return module;
}

void modules_destruct(modules *pool)
{
  size_t ref = __sync_sub_and_fetch(&module_ref, 1);

  list_destruct(&pool->modules, module_destruct);

  if (ref == 0)
  {
    assert(lt_dlexit() == 0);
  }
}

void *module_load(module *module, struct graph *graph)
{
  return module->table->load ? module->table->load(graph) : NULL;
}

void *module_create(module *module, void *node, json_t *spec)
{
  return module->table->create ? module->table->create(node, spec) : NULL;
}

void module_receive(module *module, void *state, void *message)
{
  if (module->table->receive)
    module->table->receive(state, message);
}

void module_destroy(module *module, void *state)
{
  if (module->table->destroy)
    module->table->destroy(state);
}

void module_unload(module *module, void *state)
{
  if (module->table->unload)
    module->table->unload(state);
}
