#define _GNU_SOURCE
#include <assert.h>

#include <reactor.h>

#include "flow.h"
#include "graph.h"
#include "message.h"
#include "log.h"

#define FLOW_EXPORT __attribute__ ((visibility ("default")))

/************/
/* main api */
/************/

FLOW_EXPORT void flow_construct(flow *flow, reactor_callback *callback, void *state)
{
  flow->internal = malloc(sizeof(graph));
  graph_construct(flow->internal, callback, state);
}

FLOW_EXPORT void flow_destruct(flow *flow)
{
  graph_destruct(flow->internal);
  free(flow->internal);
  flow->internal = NULL;
}

FLOW_EXPORT void flow_load(flow *flow, const char *path)
{
  graph_load(flow->internal, path);
}

/**************/
/* module api */
/**************/

FLOW_EXPORT void flow_register(void *handle, const char *name)
{
  graph_register(handle, name);
}

/* flow message */

FLOW_EXPORT void *flow_create(void *message, size_t size, int type, const flow_message_table *table)
{
  return message_create(message, size, type, table);
}

/* flow node */

FLOW_EXPORT int flow_lookup(void *handle, const char *name)
{
  return node_lookup(handle, name);
}

FLOW_EXPORT void flow_log(void *node, int level, json_t *object)
{
  log_node(node, level, object);
}

FLOW_EXPORT void flow_log_message(void *node, int level, const char *format, ...)
{
  json_t *object;
  va_list ap;
  char *message;
  int e;

  va_start(ap, format);
  e = vasprintf(&message, format, ap);
  assert(e != -1);
  object = json_object();
  json_object_set_new(object, "message", json_string(message));
  free(message);
  va_end(ap);
  flow_log(node, level, object);
  json_decref(object);
}

FLOW_EXPORT void flow_send(void *handle, void *message)
{
  node_send(handle, message);
}

FLOW_EXPORT void flow_send_and_release(void *handle, void *message)
{
  node_send(handle, message);
  message_release(message);
}

FLOW_EXPORT void flow_exit(void *handle)
{
  node_exit(handle);
}
