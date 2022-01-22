#include <stdio.h>
#include <assert.h>
#include <flow.h>

static void *load(void *graph)
{
  return (void *) 42;
}

static void unload(void *state)
{
  assert((uintptr_t) state == 42);
}

flow_module_table example_module_table = {.load = load, .unload = unload};
