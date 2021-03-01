#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <err.h>
#include <sys/epoll.h>

#include <ltdl.h>

#include <jansson.h>
#include <dynamic.h>
#include <reactor.h>

#include "message.h"
#include "flow.h"
#include "test.h"

static int flow_debug(flow *flow)
{
  return !!(flow->flags & FLOW_DEBUG);
}

/*** flow thread ***/

static core_status flow_thread_message(core_event *event)
{
  flow_node *node = event->state;
  void *message;

  message = message_receive(node->sockets[1]);
  if (node->module->table->receive)
    node->module->table->receive(node->state, message);
  message_release(message);
  return CORE_OK;
}

static void *flow_thread_start(void *arg)
{
  flow_node *node = arg;

  if (flow_debug(node->flow))
    (void) fprintf(stderr, "[debug] starting thread for node %s\n", node->name);
  reactor_construct();
  core_add(NULL, flow_thread_message, node, node->sockets[1], EPOLLIN);
  node->state = node->module->table->constructor(node->spec, node->sockets[1]);
  reactor_loop();
  reactor_destruct();
  return NULL;
}

/***/

static flow_module *flow_lookup_module(flow *flow, const char *name)
{
  flow_module *module;

  if (name)
    list_foreach(&flow->modules, module)
      if (strcmp(module->name, name) == 0)
        return module;
  return NULL;
}

static void flow_add_module(flow *flow, const char *name)
{
  lt_dladvise advise;
  flow_module *module;

  if (!name || flow_lookup_module(flow, name))
    return;

  module = list_push_back(&flow->modules, NULL, sizeof *module);
  module->name = name;

  lt_dladvise_init(&advise);
  lt_dladvise_ext(&advise);
  lt_dladvise_global(&advise);
  module->handle = lt_dlopenadvise(name, advise);
  lt_dladvise_destroy(&advise);
  if (module->handle)
    module->table = lt_dlsym(module->handle, "module_table");

  if (!module->handle ||
      !module->table ||
      !module->table->constructor)
  {
    (void) fprintf(stderr, "[error] unable to load module %s\n", name);
    list_erase(module, NULL);
    flow->errors++;
    return;
  }

  if (module->table->load)
    module->state = module->table->load();
}

static void flow_remove_module(flow_module *module)
{
  if (module->table->unload)
    module->table->unload(module->state);
  lt_dlclose(module->handle);
  list_erase(module, NULL);
}

static flow_node *flow_lookup_node(flow *flow, const char *name)
{
  flow_node *node;

  if (name)
    list_foreach(&flow->nodes, node) if (strcmp(node->name, name) == 0) return node;
  return NULL;
}

static core_status flow_node_message(core_event *event)
{
  flow_node **i, *node = event->state;
  void *message;

  message = message_receive(node->sockets[0]);
  node->stats.received++;
  list_foreach(&node->edges, i)
  {
    (*i)->stats.sent++;
    message_hold(message);
    message_send((*i)->sockets[0], message);
  }
  message_release(message);
  return CORE_OK;
}

static void flow_socket(int s)
{
  assert(fcntl(s, F_SETFL, O_NONBLOCK) == 0);
  assert(setsockopt(s, SOL_SOCKET, SO_RCVBUF, (int[]){1048576}, sizeof (int)) == 0);
  assert(setsockopt(s, SOL_SOCKET, SO_SNDBUF, (int[]){1048576}, sizeof (int)) == 0);
}

static void flow_add_node(flow *flow, const char *name, json_t *spec)
{
  flow_node *node;
  const char *module_name;
  int e;

  node = list_push_back(&flow->nodes, NULL, sizeof *node);
  list_construct(&node->edges);
  node->flow = flow;
  node->name = name;
  node->spec = spec;

  module_name = json_string_value(json_object_get(spec, "module"));
  if (!module_name)
    module_name = name;
  node->module = flow_lookup_module(flow, (char *) module_name);
  if (!node->module)
  {
    (void) fprintf(stderr, "[error] unable to add node %s\n", name);
    list_erase(node, NULL);
    flow->errors++;
    return;
  }

  e = socketpair(AF_UNIX, SOCK_SEQPACKET, PF_UNSPEC, node->sockets);
  assert(e == 0);
  flow_socket(node->sockets[0]);
  flow_socket(node->sockets[1]);

  if (flow_debug(flow))
    (void) fprintf(stderr, "[debug] creating instance of node %s\n", name);

  pthread_create(&node->thread, NULL, flow_thread_start, node);
  core_add(NULL, flow_node_message, node, node->sockets[0], EPOLLIN);
}

static void flow_remove_node(flow_node *node)
{
  // XXX this should be done by worker thread, prob by sending signal to thread
  if (node->module && node->module->table->destructor)
    node->module->table->destructor(node->state);
  list_destruct(&node->edges, NULL);
  list_erase(node, NULL);
}

static core_status flow_timer(core_event *event)
{
  flow *flow= event->state;
  flow_node *node;

  list_foreach(&flow->nodes, node)
  {
    (void) fprintf(stdout, "%snode %s = %lu/%lu",
                   node == list_front(&flow->nodes) ? "": ", ",
                   node->name, node->stats.received, node->stats.sent);
    node->stats = (flow_stats) {0};
  }
  (void) fprintf(stdout, "\n");
  return CORE_OK;
}

void flow_construct(flow *flow, core_callback *callback, void *state)
{
  *flow = (struct flow) {.user = {.callback = callback, .state = state}};
  lt_dlinit();
  lt_dlsetsearchpath("./module:/usr/lib/flow");
  list_construct(&flow->modules);
  list_construct(&flow->nodes);
  timer_construct(&flow->timer, flow_timer, flow);
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
  json_decref(flow->spec);
}

static void flow_connect(flow *flow, json_t *spec)
{
  flow_node *from, *to;
  const char *source, *target;

  source = json_string_value(json_object_get(spec, "source"));
  target = json_string_value(json_object_get(spec, "target"));
  if (flow_debug(flow))
    (void) fprintf(stderr, "[debug] connecting node %s to node %s\n", source, target);

  from = flow_lookup_node(flow, source);
  to = flow_lookup_node(flow, target);

  if (from && to)
    list_push_back(&from->edges, &to, sizeof to);
  else
  {
    (void) fprintf(stderr, "[error] unable to connect %s to %s\n", source, target);
    flow->errors++;
  }
}

void flow_configure(flow *flow, char *path)
{
  json_t *graph;
  const char *key, *name;
  json_t *value;
  size_t index;

  flow->spec = json_load_file(path, 0, NULL);
  graph = json_object_get(flow->spec, "graph");
  if (!graph)
  {
    (void) fprintf(stderr, "[error] unable to load configuration from %s\n", path);
    flow->errors++;
  }

  // check debug flag
  if (json_is_true(json_object_get(json_object_get(graph, "metadata"), "debug")))
    flow->flags |= FLOW_DEBUG;

  // load modules
  json_object_foreach(json_object_get(graph, "nodes"), key, value)
  {
    name = json_string_value(json_object_get(json_object_get(value, "metadata"), "module"));
    if (!name)
      name = key;
    if (flow_debug(flow))
      (void) fprintf(stderr, "[debug] load module %s\n", name);
    flow_add_module(flow, name);
  }

  // create nodes
  json_object_foreach(json_object_get(graph, "nodes"), key, value)
  {
    if (flow_debug(flow))
      (void) fprintf(stderr, "[debug] add node %s\n", key);
    flow_add_node(flow, (char *) key, json_object_get(value, "metadata"));
  }

  // connect nodes
  json_array_foreach(json_object_get(graph, "edges"), index, value)
    flow_connect(flow, value);

  timer_set(&flow->timer, 1000000000, 1000000000);
}
