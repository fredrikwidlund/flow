#include <stdio.h>
#include <stdlib.h>
#include <flow.h>

typedef struct state state;

struct state
{
  void   *node;
  size_t  count;
};


static void receive(void *arg, void *message)
{
  state *state = arg;

  state->count++;
}

static void *create(void *node, json_t *spec)
{
  state *state = calloc(1, sizeof *state);

  *state = (struct state) {.node = node, .count = 0};
  return state;
}

static void destroy(void *arg)
{
  state *state = arg;

  flow_log_message(state->node, FLOW_INFO, "counter received %lu messages\n", state->count);
  free(state);
}

flow_module_table counter_module_table = {.create = create, .receive = receive, .destroy = destroy};
