#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include <reactor.h>

#include "node.h"
#include "edge.h"
#include "queue.h"
#include "graph.h"
#include "message.h"
#include "log.h"

static void node_queue(reactor_event *event)
{
  node *node = event->state;
  void *message = (void *) event->data;

  switch (event->type)
  {
  case QUEUE_CLOSE:
    node_stop(node);
    break;
  case QUEUE_MESSAGE:
    module_receive(node->module, node->state, message);
    break;
  }
}

void node_construct(node *node, struct graph *graph, const char *name, const char *group, module *module, thread *thread,
                    json_t *spec)
{
  *node = (struct node) {.graph = graph, .thread = thread, .name = name, .group = group ? group : name,
    .module = module, .thread = thread, .spec = spec};
  list_construct(&node->edges);
  queue_construct(&node->queue, node_queue, node);
}

void node_add(node *source, node *target, json_t *spec)
{
  int type;

  type = symbols_lookup(&source->graph->symbols, json_string_value(json_object_get(spec, "type")));
  edge_construct(list_push_back(&source->edges, NULL, sizeof(edge)), target, type);
}

void node_start(node *node)
{
  queue_start(&node->queue);
  node->state = module_create(node->module, node, node->spec);
}

void node_stop(node *node)
{
  queue_stop(&node->queue);
  module_destroy(node->module, node->state);
}

void node_destruct(node *node)
{
  queue_destruct(&node->queue);
  list_destruct(&node->edges, NULL);
  json_decref(node->spec);
}

int node_lookup(node *node, const char *name)
{
  return symbols_lookup(&node->graph->symbols, name);
}

void node_send(node *node, void *message)
{
  edge *edge;

  list_foreach(&node->edges, edge)
  {
    if (edge->type && message_type(message) != edge->type)
      continue;

    if (node->thread == edge->target->thread)
      module_receive(edge->target->module, edge->target->state, message);
    else
      queue_send(&edge->target->queue, message);
  }
}

void node_exit(node *node)
{
  queue_send(&node->thread->queue, NULL);

  //graph_stop(node->graph);
}
