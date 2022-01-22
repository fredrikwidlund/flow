#include <assert.h>

#include <reactor.h>
#include <flow.h>

typedef struct state state;

struct state
{
  void   *handle;
  timer   timer;
  size_t  count;
  size_t  limit;
};

static void timeout(reactor_event *event)
{
  state *state = event->state;
  const int type = flow_lookup(state->handle, "number");

  state->count++;
  flow_send_and_release(state->handle, flow_create(&state->count, sizeof state->count, type, NULL));
  if (state->count < state->limit)
    return;

  timer_clear(&state->timer);
  flow_exit(state->handle);
}

static void *load(void *flow)
{
  flow_register(flow, "number");
  return NULL;
}

static void *create(void *handle, json_t *spec)
{
  state *state = malloc(sizeof *state);

  assert(json_integer_value(json_object_get(spec, "test")) == 42);

  *state = (struct state) {.handle = handle, .limit = 10};
  timer_construct(&state->timer, timeout, state);
  timer_set(&state->timer, 100000000, 100000000);

  flow_log_message(state->handle, FLOW_INFO, "generator created");
  return state;
}

static void destroy(void *arg)
{
  state *state = arg;

  flow_log_message(state->handle, FLOW_INFO, "generator destroyed");
  timer_destruct(&state->timer);
  free(state);
}

flow_module_table generator_module_table = {.load = load, .create = create, .destroy = destroy};
