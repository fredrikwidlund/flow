#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <dynamic.h>
#include <reactor.h>
#include <jansson.h>

#include "flow.h"
#include "message.h"

uintptr_t test_message = (uintptr_t) &test_message;

struct test
{
  int      socket;
  timer    t1;
  timer    t2;
  size_t   last;
  size_t   sum;
  size_t   count;
};

static void release(void *message)
{
  (void) message;
}

static core_status t1_event(core_event *event)
{
  struct test *test = event->state;

  if (test->last)
  {
    test->sum += core_now(NULL) - test->last;
    test->count++;
  }
  test->last = core_now(NULL);

  message_send(test->socket, message_create(test_message, (int[]){42}, sizeof (int), release));
  return CORE_OK;
}

static core_status t2_event(core_event *event)
{
  struct test *test = event->state;

  fprintf(stderr, "count %lu, average %f\n", test->count, (double) test->sum / test->count);
  test->count = 0;
  test->sum = 0;
  return CORE_OK;
}

static void *constructor(json_t *spec, int socket)
{
  struct test *test;

  (void) spec;

  (void) fprintf(stderr, "[test] constructor\n");
  test = malloc(sizeof *test);
  test->socket = socket;
  timer_construct(&test->t1, t1_event, test);
  timer_construct(&test->t2, t2_event, test);
  timer_set(&test->t1, 20000000, 20000000);
  timer_set(&test->t2, 1000000000, 1000000000);
  return test;
}

flow_table module_table = {.constructor = constructor};
