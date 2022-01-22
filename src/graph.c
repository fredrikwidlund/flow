#include <assert.h>
#include <string.h>
#include <jansson.h>

#include "graph.h"
#include "symbols.h"
#include "modules.h"
#include "thread.h"
#include "node.h"
#include "edge.h"
#include "log.h"

static node *graph_node(graph *graph, const char *name)
{
  node *node;

  if (!name)
    return NULL;
  list_foreach(&graph->nodes, node)
    if (strcmp(node->name, name) == 0)
      return node;
  return NULL;
}

static thread *graph_thread(graph *graph, const char *name)
{
  thread *thread;

  list_foreach(&graph->threads, thread)
    if (strcmp(thread->name, name) == 0)
      return thread;

  thread = list_push_back(&graph->threads, NULL, sizeof *thread);
  thread_construct(thread, graph, name);
  return thread;
}

static void graph_add(graph *graph, const char *name, json_t *spec, module *module)
{
  node *node;
  const char *group;
  thread *thread;

  group = json_string_value(json_object_get(spec, "group"));
  node = list_push_back(&graph->nodes, NULL, sizeof *node);
  thread = graph_thread(graph, group ? group : name);
  node_construct(node, graph, name, group, module, thread, spec);
}

static void graph_connect(graph *graph, node *source, node *target, json_t *spec)
{
  node_add(source, target, spec);
}

static void graph_start(graph *graph)
{
  thread *thread;

  queue_start(&graph->events);
  list_foreach(&graph->threads, thread)
    thread_start(thread);
}

static void graph_release_node(void *arg)
{
  node_destruct(arg);
}

static void graph_release_thread(void *arg)
{
  thread_destruct(arg);
}

static void graph_events(reactor_event *event)
{
  graph *graph = event->state;

  assert(event->type == QUEUE_MESSAGE);
  reactor_dispatch(&graph->user, FLOW_LOG, event->data);
}

void graph_construct(graph *graph, reactor_callback *callback, void *state)
{
  *graph = (struct graph) {.user = {.callback = callback, .state = state}};

  symbols_construct(&graph->symbols);
  modules_construct(&graph->modules);
  queue_construct(&graph->events, graph_events, graph);
  list_construct(&graph->threads);
  list_construct(&graph->nodes);
}

void graph_load(graph *graph, const char *path)
{
  string s;
  data prefix, postfix;
  char *value;
  json_t *spec;
  json_error_t error;

  s = string_load(path);
  if (string_empty(s))
    log_critical(graph, "unable to load graph from %s", path);

  while (1)
  {
    prefix = string_find_data(s, data_string("${"));
    if (data_empty(prefix))
      break;
    postfix = string_find_at_data(s, data_offset(string_data(s), prefix) + data_size(prefix), data_string("}"));
    if (data_empty(postfix))
      break;
    ((char *) data_base(postfix))[0] = 0;
    value = getenv((char *) data_base(prefix) + data_size(prefix));
    if (value)
      s = string_replace_data(s, data_merge(prefix, postfix), data_string(value));
  }

  spec = json_loads(s, 0, &error);
  string_free(s);
  if (!spec)
    log_critical(graph, "unable to parse json, %s", error.text);

  graph_open(graph, spec);
  json_decref(spec);
}

void graph_open(graph *graph, json_t *spec)
{
  json_t *metadata, *value, *node_spec;
  size_t index;
  const char *key, *name;
  module *module;

  graph->spec = json_object_get(spec, "graph");
  json_incref(graph->spec);
  metadata = json_object_get(graph->spec, "metadata");

  /* add module search paths */
  json_array_foreach(json_object_get(metadata, "search"), index, value)
    modules_search(&graph->modules, json_string_value(value));

  /* load modules */
  json_array_foreach(json_object_get(metadata, "modules"), index, value)
    modules_load(&graph->modules, graph, json_string_value(value));

  /* add nodes */
  json_object_foreach(json_object_get(graph->spec, "nodes"), key, value)
  {
    node_spec = json_object();
    json_object_update(node_spec, json_object_get(metadata, "globals"));
    json_object_update(node_spec, json_object_get(value, "metadata"));
    name = json_string_value(json_object_get(node_spec, "module"));
    module = name ? modules_load(&graph->modules, graph, name) : modules_match(&graph->modules, graph, key);
    graph_add(graph, key, node_spec, module);
  }

  /* connect nodes */
  json_array_foreach(json_object_get(graph->spec, "edges"), index, value)
    graph_connect(graph,
                  graph_node(graph, json_string_value(json_object_get(value, "source"))),
                  graph_node(graph, json_string_value(json_object_get(value, "target"))),
                  json_object_get(value, "metadata"));

  /* start flow */
  graph_start(graph);
}

void graph_register(graph *graph, const char *name)
{
  symbols_register(&graph->symbols, name);
}

void graph_stop(graph *graph)
{
  thread *thread;

  list_foreach(&graph->threads, thread)
    thread_stop(thread);
  queue_read(&graph->events);
  queue_stop(&graph->events);
}

void graph_destruct(graph *graph)
{
  graph_stop(graph);
  list_destruct(&graph->nodes, graph_release_node);
  list_destruct(&graph->threads, graph_release_thread);
  modules_destruct(&graph->modules);
  symbols_destruct(&graph->symbols);
  json_decref(graph->spec);
  *graph = (struct graph) {0};
}
