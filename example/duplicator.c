#include <stdio.h>
#include <stdlib.h>
#include <flow.h>

typedef struct state state;

struct state
{
  void   *node;
  size_t  copies;
};

static void receive(void *arg, void *message)
{
  state *state = arg;
  size_t i;

  for (i = 0; i < state->copies; i++)
    flow_send(state->node, message);
}

static void *create(void *node, json_t *spec)
{
  state *state = calloc(1, sizeof *state);

  *state = (struct state) {.node = node, .copies = 1000};
  return state;
}

static void destroy(void *arg)
{
  state *state = arg;

  flow_log_message(state->node, FLOW_INFO, "duplicator destroyed");
  free(state);
}

flow_module_table duplicator_module_table = {.create = create, .receive = receive, .destroy = destroy};
