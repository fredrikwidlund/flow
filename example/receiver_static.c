#include <stdio.h>

#include "flow.h"

static void receive(void *instance, void *message)
{
  int *count = message;

  (void) instance;
  printf("%d\n", *count);
  fflush(stdout);
}

flow_module_table receiver_module_table = {.receive = receive};
