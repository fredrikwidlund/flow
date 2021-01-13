#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <dlfcn.h>

#include <dynamic.h>
#include <reactor.h>
#include <jansson.h>

#include "flow.h"

static flow_node *flow_lookup_node(flow *flow, const char *id)
{
  flow_node *node;

  list_foreach(&flow->nodes, node) if (strcmp(node->id, id) == 0) return node;

  return NULL;
}

static flow_module *flow_load_module(flow *flow, const char *name)
{
  flow_module *m;
  char path[PATH_MAX];

  list_foreach(&flow->modules, m) if (strcmp(m->name, name) == 0) return m;

  (void) fprintf(stderr, "[load %s]\n", name);
  m = list_push_back(&flow->modules, NULL, sizeof *m);
  m->name = strdup(name);
  snprintf(path, sizeof path, "module/.libs/%s.so", name);
  m->object = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
  if (!m->object)
    return NULL;

  m->handlers = dlsym(m->object, "module_handlers");
  if (!m->handlers)
    return NULL;

  if (m->handlers->load)
    m->state = m->handlers->load();

  return m;
}

void flow_construct(flow *flow)
{
  *flow = (struct flow) {0};
  list_construct(&flow->modules);
  list_construct(&flow->nodes);
}

void flow_destruct(flow *flow)
{
  json_decref(flow->spec);
}

int flow_load(flow *flow, char *path)
{
  return flow_configure(flow, json_load_file(path, 0, NULL));
}

int flow_configure(flow *flow, json_t *spec)
{
  const char *name, *key;
  json_t *value;
  flow_module *module;
  flow_node *node, *source, *target;
  flow_edge *edge;
  size_t index;
  int e;

  if (!spec)
    return -1;

  flow->spec = spec;
  json_object_foreach(json_object_get(json_object_get(flow->spec, "graph"), "nodes"), key, value)
  {
    name = json_string_value(json_object_get(json_object_get(value, "metadata"), "module"));
    if (!name)
      name = key;
    module = flow_load_module(flow, name);
    if (!module)
      return -1;

    (void) fprintf(stderr, "[create %s]\n", key);
    node = list_push_back(&flow->nodes, NULL, sizeof *node);
    node->module = module;
    node->id = key;
    list_construct(&node->edges);

    if (node->module->handlers->create)
    {
      node->state = node->module->handlers->create(node, json_object_get(value, "metadata"));
      if (!node->state)
        return -1;
    }
  }

  json_array_foreach(json_object_get(json_object_get(flow->spec, "graph"), "edges"), index, value)
  {
    source = flow_lookup_node(flow, json_string_value(json_object_get(value, "source")));
    target = flow_lookup_node(flow, json_string_value(json_object_get(value, "target")));
    if (!source || !target)
      return -1;

    (void) fprintf(stderr, "[connect %s -> %s]\n", source->id, target->id);
    edge = list_push_back(&source->edges, NULL, sizeof *edge);
    edge->type = json_string_value(json_object_get(json_object_get(value, "metadata"), "type"));
    edge->target = target;
  }

  list_foreach(&flow->nodes, node) if (node->module->handlers->start)
  {
    e = node->module->handlers->start(node->state);
    if (e == -1)
      return -1;
  }

  return 0;
}

void flow_send(flow_node *node, void *data)
{
  flow_edge *edge;

  list_foreach(&node->edges, edge) if (edge->target->module->handlers->receive)
      edge->target->module->handlers->receive(edge->target->state, data);
}
