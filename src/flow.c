 #define _GNU_SOURCE

#include <assert.h>
#include <string.h>
#include <unistd.h>

#include <ltdl.h>

#include <jansson.h>
#include <dynamic.h>
#include <reactor.h>

#include "flow.h"

static core_status flow_stats_event(core_event *event)
{
  flow *flow = event->state;
  flow_node *node;
  json_t *stats, *o;
  core_status e;

  assert(event->type == TIMER_ALARM);

  stats = json_object();
  list_foreach(&flow->nodes, node)
  {
    o = json_object();
    json_object_set_new(o, "received", json_integer(node->received));
    json_object_set_new(o, "sent", json_integer(node->sent));
    json_object_set_new(stats, node->name, o);
    node->received = 0;
    node->sent = 0;
  }
  e = core_dispatch(&flow->user, FLOW_STATS, (uintptr_t) stats);
  json_decref(stats);
  return e;
}

void flow_construct(flow *flow, core_callback *callback, void *state)
{
  *flow = (struct flow) {.user = {.callback = callback, .state = state}};

  flow_log(flow, FLOW_DEBUG, "constructing flow");
  flow_modules_construct(&flow->modules);
  flow_nodes_construct(flow);
  maps_construct(&flow->symbols);
  timer_construct(&flow->timer, flow_stats_event, flow);
}

void flow_open(flow *flow, json_t *spec)
{
  json_t *root, *metadata;
  const char *name, *key;
  json_t *value;
  size_t index;

  flow_log(flow, FLOW_DEBUG, "configuring flow");

  root = json_object_get(spec, "graph");
  metadata = json_object_get(root, "metadata");

  flow_stats(flow, json_is_true(json_object_get(metadata, "stats")));

  name = json_string_value(json_object_get(metadata, "name"));
  assert(name ? asprintf(&flow->name, "%s", name) : asprintf(&flow->name, "%d", getpid()) != -1);

  /* add module search paths */
  json_array_foreach(json_object_get(metadata, "search"), index, value)
    flow_search(flow, json_string_value(value));

  /* load modules */
  json_array_foreach(json_object_get(metadata, "modules"), index, value)
      flow_load(flow, json_string_value(value));

  /* load module dependencies */
  json_object_foreach(json_object_get(root, "nodes"), key, value)
  {
    name = json_string_value(json_object_get(json_object_get(value, "metadata"), "module"));
    if (name)
      flow_load(flow, name);
    else if (!flow_modules_match_prefix(&flow->modules, key))
      flow_load(flow, key);
  }

  /* add nodes */
  json_object_foreach(json_object_get(root, "nodes"), key, value)
    flow_add(flow, key, json_object_get(value, "metadata"));

  /* connect nodes */
  json_array_foreach(json_object_get(root, "edges"), index, value)
    flow_connect(flow,
                 json_string_value(json_object_get(value, "source")),
                 json_string_value(json_object_get(value, "target")),
                 json_object_get(value, "metadata"));
}

void flow_close(flow *flow)
{
  flow_log(flow, FLOW_DEBUG, "stopping flow");
  flow_nodes_stop(flow);
}

void flow_destruct(flow *flow)
{
  flow_log(flow, FLOW_DEBUG, "destructing flow");
  flow_close(flow);
  flow_nodes_destruct(flow);
  flow_modules_destruct(flow);
  maps_destruct(&flow->symbols, NULL);
  free(flow->name);
}

void flow_stats(flow *flow, int flag)
{
  if (flag)
    timer_set(&flow->timer, 1000000000, 1000000000);
}

void flow_search(flow *flow, const char *path)
{
  flow_log(flow, FLOW_DEBUG, "adding path %s to module search paths", path);
  flow_modules_search(path);
}

void flow_load(flow *flow, const char *name)
{
  flow_log(flow, FLOW_DEBUG, "adding module %s", name);
  flow_modules_load(flow, name);
}

void flow_register(flow *flow, const char *name)
{
  flow_log(flow, FLOW_DEBUG, "adding module %s", name);
  flow_modules_register(flow, name);
}

void flow_add(flow *flow, const char *name, json_t *spec)
{
  flow_log(flow, FLOW_DEBUG, "adding node %s", name);
  flow_nodes_add(flow, name, spec);
}

void flow_connect(flow *flow, const char *source, const char *target, json_t *spec)
{
  flow_log(flow, FLOW_DEBUG, "connecting node %s to node %s", source, target);
  flow_nodes_connect(flow, source, target, spec);
}

void flow_exit(flow_node *node)
{
  flow_log(node->flow, FLOW_DEBUG, "node %s exiting", node->name);
  flow_node_exit(node);
}

void *flow_create(void *message, size_t size, int type, const flow_table *table)
{
  return flow_message_create(message, size, type, table);
}

void flow_send(flow_node *node, void *message)
{
  flow_node_send(node, message);
}

void flow_send_and_release(flow_node *node, void *message)
{
  flow_node_send(node, message);
  flow_message_release(message);
}

void flow_release(void *message)
{
  flow_message_release(message);
}

int flow_type(void *message)
{
  return flow_message_type(message);
}

int flow_symbol(flow *flow, const char *name)
{
  int id;

  id = maps_at(&flow->symbols, (char *) name);
  if (id)
    return id;
  flow->symbol_count++;
  maps_insert(&flow->symbols, (char *) name, flow->symbol_count, NULL);
  return flow->symbol_count;
}
