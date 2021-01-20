#include <dynamic.h>
#include <reactor.h>
#include <jansson.h>

#include "flow.h"

struct state
{
  timer timer;
  size_t counter;
};

static core_status event(core_event *event)
{
  struct state *state = event->state;

  state->counter++;
  return CORE_OK;
}

static core_status timeout(core_event *event)
{
  struct state *state = event->state;
  core_counters *counters;

  counters = core_get_counters(NULL);
  (void) fprintf(stderr, "[statistics] %lu events/second, %.02f/%.02fGHz (%.02f%%)\n", state->counter,
                 (double) (counters->awake) / 1000000000.0, (double) (counters->awake + counters->sleep) / 1000000000.0,
                 100. * (double) counters->awake / (double) (counters->awake + counters->sleep));
  core_clear_counters(NULL);
  state->counter = 0;
  return CORE_OK;
}

static void *new (core_callback *user_callback, void *user_state, json_t *spec)
{
  struct state *state;

  (void) user_callback;
  (void) user_state;
  (void) spec;

  state = malloc(sizeof *state);
  *state = (struct state) {0};
  timer_construct(&state->timer, timeout, state);
  return state;
}

static void start(void *arg)
{
  struct state *state = arg;

  timer_set(&state->timer, 1000000000, 1000000000);
}

flow_table module_table = {.new = new, .start = start, .event = event};
