#define _GNU_SOURCE

#include <stdio.h>
#include <assert.h>

#include <reactor.h>

#include "graph.h"
#include "node.h"
#include "message.h"
#include "log.h"

static void log_event_destroy(void *message)
{
  log_event *event = message;

  json_decref(event->object);
}

static const message_table log_event_table = {.destroy = log_event_destroy};

static void log_level(json_t *object, int level)
{
  const char *severity[] = {"emerg", "alert", "crit", "error", "warning", "notice", "info", "debug"};

  assert(level >= 0 && level <= 7);
  json_object_set_new(object, "severity", json_string(severity[level]));
}

void log_graph(struct graph *graph, int level, const char *format, ...)
{
  va_list ap;
  char *message;
  log_event event;
  int e;

  event.object = json_object();
  log_level(event.object, level);
  va_start(ap, format);
  e = vasprintf(&message, format, ap);
  assert(e != -1);
  json_object_set_new(event.object, "message", json_string(message));
  free(message);
  va_end(ap);
  reactor_dispatch(&graph->user, FLOW_LOG, (uintptr_t) &event);
  json_decref(event.object);
  if (level <= FLOW_CRIT)
    exit(1);
}

void log_node(node *node, int level, json_t *object)
{
  log_event *event;

  event = message_create(NULL, sizeof *event, 0, &log_event_table);
  event->object = json_object();
  json_object_set_new(event->object, "node", json_string(node->name));
  log_level(event->object, level);
  json_object_update(event->object, object);
  queue_send(&node->graph->events, event);
  message_release(event);
  if (level <= FLOW_CRIT)
    exit(1);
}
