#include <stdio.h>
#include <assert.h>

#include <dynamic.h>
#include <reactor.h>
#include <jansson.h>

#include "flow.h"

struct sender
{
  void *handle;
  int count;
  timer timer;
};

static core_status timeout(core_event *event)
{
  struct sender *sender = event->state;

  assert(event->type == TIMER_ALARM);
  if (sender->count > 3)
    flow_exit(sender->handle);
  else
    flow_send_and_release(sender->handle, flow_create((int[]){sender->count}, sizeof(int), 0, NULL));
  sender->count++;
  return CORE_OK;
}

static void *create(void *handle, json_t *spec)
{
  struct sender *sender;

  (void) spec;
  sender = calloc(1, sizeof *sender);
  sender->handle = handle;
  timer_construct(&sender->timer, timeout, sender);
  timer_set(&sender->timer, 10000000, 10000000);

  return sender;
}

static void destroy(void *instance)
{
  struct sender *sender = instance;

  timer_destruct(&sender->timer);
  free(sender);
}

flow_module_table sender_module_table = {.create = create, .destroy = destroy};
