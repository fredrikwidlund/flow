#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <jansson.h>

#include "flow.h"
#include "flow_log.h"

static void flow_log_event_destroy(void *message)
{
  flow_log_event *log = message;

  json_decref(log->event);
}

static const flow_table flow_log_event_table = {.destroy = flow_log_event_destroy};


flow_log_event *flow_log_create(const char *name, flow_log_level level, const char *key, json_t *value)
{
  const char *severities[] = {"emerg", "alert", "crit", "error", "warning", "notice", "info", "debug"};
  flow_log_event *log;
  json_t *event;

  assert(level >= FLOW_LOG_EMERG && level <= FLOW_LOG_DEBUG);

  event = json_object();
  json_object_set_new(event, "type", json_string("log"));
  json_object_set_new(event, "severity", json_string(severities[level]));
  if (name)
    json_object_set_new(event, "node", json_string(name));
  json_incref(value);
  json_object_set_new(event, key, value);

  log = flow_create(NULL, sizeof *log, 0, &flow_log_event_table);
  log->event = event;
  return log;
}

void flow_log(flow_node *node, flow_log_level level, const char *name, json_t *value)
{
  flow_log_event *log;

  if (level == FLOW_LOG_DEBUG && node->flow->debug == 0)
    return;

  log = flow_log_create(node->name, level, name, value);
  flow_queue_send(&node->flow->events_send, log);
  flow_release(log);
}

void flow_log_message(flow_node *node, flow_log_level level, const char *format, ...)
{
  va_list ap;
  char *line;
  json_t *message;

  va_start(ap, format);
  assert(vasprintf(&line, format, ap) != -1);
  message = json_string(line);
  flow_log(node, level, "message", message);
  json_decref(message);
  free(line);
}

void flow_log_sync(flow *flow, flow_log_level level, const char *name, json_t *value)
{
  flow_log_event *log;

  if (level == FLOW_LOG_DEBUG && flow->debug == 0)
    return;

  log = flow_log_create(NULL, level, name, value);
  reactor_dispatch(&flow->handler, FLOW_EVENT, (uintptr_t) log);
  flow_release(log);
  if (level <= FLOW_LOG_CRIT)
  {
    reactor_dispatch(&flow->handler, FLOW_ERROR, 0);
    exit(1);
  }
}

void flow_log_sync_message(flow *flow, flow_log_level level, const char *format, ...)
{
  va_list ap;
  char *line;
  json_t *message;

  va_start(ap, format);
  assert(vasprintf(&line, format, ap) != -1);
  message = json_string(line);
  free(line);
  flow_log_sync(flow, level, "message", message);
  json_decref(message);
}
