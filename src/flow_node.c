#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <signal.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <dlfcn.h>
#include <err.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/epoll.h>

#include <ltdl.h>

#include <jansson.h>
#include <dynamic.h>
#include <reactor.h>

#include "flow.h"
#include "flow_module.h"
#include "flow_node.h"
#include "flow_message.h"
#include "flow_log.h"
#include "flow_queue.h"

/*************/
/* flow node */
/*************/

static core_status flow_node_thread_receive(core_event *event)
{
  flow_node *node = event->state;
  void *message = (void *) event->data;

  switch (event->type)
  {
  case FLOW_QUEUE_END:
    flow_queue_unlisten(&node->tail);
    flow_module_destroy(node->module, node->state);
    return CORE_ABORT;
  case FLOW_QUEUE_MESSAGE:
    node->received++;
    flow_module_receive(node->module, node->state, message);
    flow_message_release(message);
    return CORE_OK;
  default:
    flow_log(node->flow, FLOW_CRITICAL, "unknown queue event %d", event->type);
    return CORE_ABORT;
  }
}

static void *flow_node_thread(void *arg)
{
  flow_node *node = arg;
  char name[256];

  (void) snprintf(name, sizeof name, "[%s]%s", node->flow->name, node->name);
  (void) pthread_setname_np(node->thread, name);

  reactor_construct();
  flow_queue_listen(&node->tail, flow_node_thread_receive, node);
  node->state = flow_module_create(node->module, node, node->metadata);
  reactor_loop();
  reactor_destruct();
  return NULL;
}

static core_status flow_node_receive(core_event *event)
{
  flow_node *node = event->state, **i;
  void *message = (void *) event->data;

  switch (event->type)
  {
  case FLOW_QUEUE_END:
    flow_log(node->flow, FLOW_DEBUG, "received shutdown from %s", node->name);
    flow_close(node->flow);
    return CORE_OK;
  case FLOW_QUEUE_MESSAGE:
    list_foreach(&node->edges, i) if (!(*i)->detached)
    {
      (*i)->received++;
      flow_module_receive((*i)->module, (*i)->state, message);
    }
    flow_message_release(message);
    return CORE_OK;
  default:
    flow_log(node->flow, FLOW_CRITICAL, "unknown queue event %d", event->type);
    return CORE_ABORT;
  }
}

void flow_node_construct(flow_node *node, flow *flow, flow_module *module, const char *name, json_t *spec)
{
  node->flow = flow;
  node->name = strdup(name);
  node->metadata = spec;
  node->detached = !json_is_false(json_object_get(spec, "detached"));
  node->module = module;
  list_construct(&node->edges);

  if (node->detached)
  {
    flow_queue_construct(&node->head, &node->tail);
    flow_queue_listen(&node->head, flow_node_receive, node);
    pthread_create(&node->thread, NULL, flow_node_thread, node);
  }
  else
  {
    node->state = flow_module_create(node->module, node, spec);
  }
}

void flow_node_send(flow_node *node, void *message)
{
  flow_edge *e;

  if (node->detached && node->attached_neighbours)
    flow_queue_send(&node->tail, message);

  if (node->detached_neighbours)
  {
    list_foreach(&node->edges, e)
    {
      if (e->target->detached && (e->type == 0 || e->type == flow_type(message)))
        flow_queue_send(&e->target->head, message);
    }
  }

  /* send sync messages direct */
  if (!node->detached && node->attached_neighbours)
  {
    list_foreach(&node->edges, e)
    {
      if (!e->target->detached && (e->type == 0 || e->type == flow_type(message)))
      {
        e->target->received++;
        flow_module_receive(e->target->module, e->target->state, message);
      }
    }
  }

  node->sent++;
}

void flow_node_exit(flow_node *node)
{
  if (node->detached)
    flow_queue_send(&node->tail, NULL);
  else
    flow_close(node->flow);
}

void flow_node_stop(flow_node *node)
{
  if (node->detached)
  {
    if (node->thread)
    {
      flow_queue_send(&node->head, NULL);
      pthread_join(node->thread, NULL);
      flow_queue_destruct(&node->head);
      flow_queue_destruct(&node->tail);
      node->thread = 0;
    }
  }
  else
  {
    if (node->state)
    {
      flow_module_destroy(node->module, node->state);
      node->state = NULL;
    }
  }
}

/**************/
/* flow nodes */
/**************/

static flow_node *flow_nodes_lookup(flow *flow, const char *name)
{
  flow_nodes *nodes = &flow->nodes;
  flow_node *node;

  if (name)
    list_foreach(nodes, node) if (strcmp(node->name, name) == 0)
      return node;
  flow_log(flow, FLOW_CRITICAL, "unable to lookup node %s", name);
  return NULL;
}

void flow_nodes_construct(flow *flow)
{
  flow_nodes *nodes = &flow->nodes;
  list_construct(nodes);
}

void flow_nodes_add(flow *flow, const char *name, json_t *spec)
{
  flow_nodes *nodes = &flow->nodes;
  const char *module_name;
  flow_module *module;
  flow_node *node;

  module_name = json_string_value(json_object_get(spec, "module"));
  if (module_name)
    module = flow_modules_lookup(&flow->modules, module_name);
  else
    module = flow_modules_match_prefix(&flow->modules, name);
  if (!module)
    flow_log(flow, FLOW_CRITICAL, "unable to find module for node %s", name);
  flow_node_construct(list_push_back(nodes, NULL, sizeof *node), flow, module, name, spec);
}

void flow_nodes_connect(flow *flow, const char *source, const char *target, json_t *spec)
{
  flow_node *s, *t;
  flow_edge *e;
  const char *type;

  s = flow_nodes_lookup(flow, source);
  t = flow_nodes_lookup(flow, target);
  e = list_push_back(&s->edges, NULL, sizeof *e);
  e->target = t;
  e->spec = spec;
  type = json_string_value(json_object_get(spec, "type"));
  if (type)
    e->type = flow_symbol(flow, type);
  if (t->detached)
    s->detached_neighbours++;
  else
    s->attached_neighbours++;
}

void flow_nodes_stop(flow *flow)
{
  flow_nodes *nodes = &flow->nodes;
  flow_node *node;

  flow_log(flow, FLOW_DEBUG, "stopping nodes");
  list_foreach(nodes, node)
    flow_node_stop(node);
}

void flow_nodes_destruct(flow *flow)
{
  flow_nodes *nodes = &flow->nodes;
  flow_node *node;

  list_foreach(nodes, node)
  {
    flow_log(flow, FLOW_DEBUG, "removing node %s", node->name);
    free(node->name);
    list_destruct(&node->edges, NULL);
  }
  list_destruct(nodes, NULL);
}
