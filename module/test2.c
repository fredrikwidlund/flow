#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <dynamic.h>
#include <reactor.h>
#include <jansson.h>

#include "flow.h"
#include "message.h"

#include "test.h"

static void *constructor(json_t *spec, int socket)
{
  (void) spec;
  (void) socket;
  return NULL;
}

static void receive(void *state, void *message)
{
  (void) state;
  (void) message;
}

flow_table module_table = {.constructor = constructor, .receive = receive};
