#include <stdio.h>

#include "flow.h"

struct state
{
  void *node;
};

static void receive(void *node, void *message)
{
  int *count = message;
  json_t *json;

  json = json_integer(*count);
  flow_log(node, FLOW_LOG_INFO, "count", json);
  json_decref(json);
}

static void *create(void *node, json_t *spec)
{
  (void) spec;
  return node;
}

flow_module_table receiver_module_table = {.create = create, .receive = receive};
