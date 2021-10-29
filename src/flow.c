#define _GNU_SOURCE

#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <ltdl.h>

#include <jansson.h>
#include <dynamic.h>
#include <reactor.h>

#include "flow.h"

static core_status flow_events_event(core_event *event)
{
  flow *flow = event->state;
  flow_log_event *log = (flow_log_event *) event->data;

  assert(event->type == FLOW_QUEUE_MESSAGE);
  core_dispatch(&flow->user, FLOW_EVENT, (uintptr_t) log);
  flow_release(log);
  return CORE_OK;
}

static core_status flow_stats_event(core_event *event)
{
  flow *flow = event->state;
  flow_node *node;
  json_t *stats, *o;

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
  flow_log_sync(flow, FLOW_LOG_INFO, "stats", stats);
  json_decref(stats);

  return CORE_OK;
}

void flow_construct(flow *flow, core_callback *callback, void *state)
{
  *flow = (struct flow) {.user = {.callback = callback, .state = state}};

  flow_queue_construct(&flow->events_receive, &flow->events_send);
  flow_modules_construct(&flow->modules);
  flow_nodes_construct(flow);
  maps_construct(&flow->symbols);
  timer_construct(&flow->timer, flow_stats_event, flow);
}

void flow_open(flow *flow, json_t *spec)
{
  json_t *metadata, *node_spec;
  const char *name, *key;
  json_t *value;
  size_t index;

  flow_queue_listen(&flow->events_receive, flow_events_event, flow);
  flow_log_sync_message(flow, FLOW_LOG_DEBUG, "configuring flow");

  flow->graph = json_deep_copy(json_object_get(spec, "graph"));
  metadata = json_object_get(flow->graph, "metadata");

  flow->debug = json_is_true(json_object_get(metadata, "debug"));
  flow_stats(flow, json_is_true(json_object_get(metadata, "stats")));

  name = json_string_value(json_object_get(metadata, "name"));
  assert(name ? asprintf(&flow->name, "%s", name) : asprintf(&flow->name, "flow") != -1);
  (void) pthread_setname_np(pthread_self(), flow->name);

  /* add module search paths */
  json_array_foreach(json_object_get(metadata, "search"), index, value)
    flow_search(flow, json_string_value(value));

  /* load modules */
  json_array_foreach(json_object_get(metadata, "modules"), index, value)
      flow_load(flow, json_string_value(value));

  /* load module dependencies */
  json_object_foreach(json_object_get(flow->graph, "nodes"), key, value)
  {
    name = json_string_value(json_object_get(json_object_get(value, "metadata"), "module"));
    if (name)
      flow_load(flow, name);
    else if (!flow_modules_match_prefix(&flow->modules, key))
      flow_load(flow, key);
  }

  /* add nodes */
  json_object_foreach(json_object_get(flow->graph, "nodes"), key, value)
    {
      node_spec = json_deep_copy(json_object_get(metadata, "globals"));
      json_object_update(node_spec, json_object_get(value, "metadata"));
      flow_add(flow, key, node_spec);
    }

  /* connect nodes */
  json_array_foreach(json_object_get(flow->graph, "edges"), index, value)
    flow_connect(flow,
                 json_string_value(json_object_get(value, "source")),
                 json_string_value(json_object_get(value, "target")),
                 json_object_get(value, "metadata"));
}

void flow_close(flow *flow)
{
  flow_log_sync_message(flow, FLOW_LOG_DEBUG, "stopping flow");
  flow_nodes_stop(flow);
  flow_queue_unlisten(&flow->events_receive);
}

void flow_destruct(flow *flow)
{
  // flow_close(flow); XXX should already be closed
  flow_nodes_destruct(flow);
  flow_modules_destruct(flow);
  maps_destruct(&flow->symbols, NULL);
  free(flow->name);
  flow_queue_destruct(&flow->events_receive);
  flow_queue_destruct(&flow->events_send);
  json_decref(flow->graph);
}

void flow_stats(flow *flow, int flag)
{
  if (flag)
    timer_set(&flow->timer, 1000000000, 1000000000);
}

void flow_search(flow *flow, const char *path)
{
  flow_log_sync_message(flow, FLOW_LOG_DEBUG, "adding path %s to module search paths", path);
  flow_modules_search(path);
}

void flow_load(flow *flow, const char *name)
{
  flow_log_sync_message(flow, FLOW_LOG_DEBUG, "adding module %s", name);
  flow_modules_load(flow, name);
}

void flow_register(flow *flow, const char *name)
{
  flow_log_sync_message(flow, FLOW_LOG_DEBUG, "adding module %s", name);
  flow_modules_register(flow, name);
}

void flow_add(flow *flow, const char *name, json_t *spec)
{
  flow_log_sync_message(flow, FLOW_LOG_DEBUG, "adding node %s", name);
  flow_nodes_add(flow, name, spec);
}

void flow_connect(flow *flow, const char *source, const char *target, json_t *spec)
{
  flow_log_sync_message(flow, FLOW_LOG_DEBUG, "connecting node %s to node %s", source, target);
  flow_nodes_connect(flow, source, target, spec);
}

void flow_abort(flow_node *node, const char *format, ...)
{
  va_list ap;
  char *line;
  json_t *message;

  va_start(ap, format);
  assert(vasprintf(&line, format, ap) != -1);
  message = json_string(line);
  free(line);
  flow_log_sync(node->flow, FLOW_LOG_CRIT, "message", message);
  json_decref(message);
  abort();
}

void flow_exit(flow_node *node)
{
  flow_log_sync_message(node->flow, FLOW_LOG_DEBUG, "node %s exiting", node->name);
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

void flow_hold(void *message)
{
  flow_message_hold(message);
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
