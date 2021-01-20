#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <dlfcn.h>
#include <err.h>

#include <ltdl.h>

#include <jansson.h>
#include <dynamic.h>
#include <reactor.h>

#include "flow.h"

static flow_module *flow_lookup_module(flow *flow, char *name)
{
  flow_module *module;

  if (name)
    list_foreach(&flow->modules, module) if (strcmp(module->name, name) == 0) return module;
  return NULL;
}

static void flow_add_module(flow *flow, char *name)
{
  lt_dladvise advise;
  flow_module *module;
  char path[PATH_MAX];

  if (!name || flow_lookup_module(flow, name))
    return;

  module = list_push_back(&flow->modules, NULL, sizeof *module);
  module->name = strdup(name);

  snprintf(path, sizeof path, "%s", name);

  lt_dladvise_init(&advise);
  lt_dladvise_ext(&advise);
  lt_dladvise_global(&advise);
  module->handle = lt_dlopenadvise(path, advise);
  lt_dladvise_destroy(&advise);
  assert(module->handle);

  flow->errors += module->handle == NULL;
  module->table = lt_dlsym(module->handle, "module_table");
  if (module->table->load)
    module->state = module->table->load();
}

static void flow_remove_module(flow_module *module)
{
  if (module->table->unload)
    module->table->unload(module->state);
  lt_dlclose(module->handle);
  free(module->name);
  list_erase(module, NULL);
}

static flow_node *flow_lookup_node(flow *flow, char *name)
{
  flow_node *node;

  if (name)
    list_foreach(&flow->nodes, node) if (strcmp(node->name, name) == 0) return node;
  return NULL;
}

static core_status flow_node_event(core_event *event)
{
  flow_node *source = event->state, **i;

  list_foreach(&source->edges, i) core_dispatch(
      (core_handler[]) {{.callback = (*i)->module->table->event, .state = (*i)->state}}, event->type, event->data);
  return CORE_OK;
}

static void flow_add_node(flow *flow, char *name, json_t *spec)
{
  flow_node *node;
  const char *module_name;

  node = list_push_back(&flow->nodes, NULL, sizeof *node);
  list_construct(&node->edges);
  node->name = strdup(name);

  module_name = json_string_value(json_object_get(spec, "module"));
  if (!module_name)
    module_name = name;
  node->module = flow_lookup_module(flow, (char *) module_name);
  assert(node->module);

  if (node->module && node->module->table->new)
    node->state = node->module->table->new (flow_node_event, node, spec);
  flow->errors += node->module == NULL;
}

static void flow_remove_node(flow_node *node)
{
  if (node->module && node->module->table->release)
    node->module->table->release(node->state);
  free(node->name);
  list_destruct(&node->edges, NULL);
  list_erase(node, NULL);
}

void flow_construct(flow *flow, core_callback *callback, void *state)
{
  lt_dlinit();
  lt_dlsetsearchpath("./module:/usr/lib/flow");
  *flow = (struct flow) {.user = {.callback = callback, .state = state}};
  list_construct(&flow->modules);
  list_construct(&flow->nodes);
}

void flow_destruct(flow *flow)
{
  while (!list_empty(&flow->nodes))
    flow_remove_node(list_front(&flow->nodes));
  list_destruct(&flow->nodes, NULL);
  while (!list_empty(&flow->modules))
    flow_remove_module(list_front(&flow->modules));
  list_destruct(&flow->modules, NULL);
  lt_dlexit();
}

static void flow_connect(flow *flow, json_t *spec)
{
  flow_node *source, *target;

  source = flow_lookup_node(flow, (char *) json_string_value(json_object_get(spec, "source")));
  target = flow_lookup_node(flow, (char *) json_string_value(json_object_get(spec, "target")));
  if (source && target)
    list_push_back(&source->edges, &target, sizeof target);
  else
    flow->errors++;
}

void flow_configure(flow *flow, json_t *spec)
{
  flow_node *node;
  const char *key, *name;
  json_t *value;
  size_t index;

  // load modules
  json_object_foreach(json_object_get(json_object_get(spec, "graph"), "nodes"), key, value)
  {
    name = json_string_value(json_object_get(json_object_get(value, "metadata"), "module"));
    flow_add_module(flow, (char *) (name ? name : key));
  }

  // create nodes
  json_object_foreach(json_object_get(json_object_get(spec, "graph"), "nodes"), key, value)
  {
    flow_add_node(flow, (char *) key, json_object_get(value, "metadata"));
  }

  // connect nodes
  json_array_foreach(json_object_get(json_object_get(spec, "graph"), "edges"), index, value)
  {
    flow_connect(flow, value);
  }

  // start nodes
  list_foreach(&flow->nodes, node)
  {
    if (node->module->table->start)
      node->module->table->start(node->state);
  }
}

int flow_global_type(void)
{
  static _Atomic int type = 0;

  type++;
  return type;
}
