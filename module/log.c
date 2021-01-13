#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

#include <dynamic.h>
#include <reactor.h>
#include <jansson.h>

#include "flow.h"

void receive(void *state, char *type, void *data)
{
  (void) state;
  if (strcmp(type, "json") == 0)
  {
    json_dumpf(data, stdout, 0);
    putc('\n', stdout);
    fflush(stdout);
  }
}

flow_module_handlers module_handlers = {.receive = receive};
