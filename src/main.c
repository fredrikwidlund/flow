#include <stdio.h>
#include <signal.h>
#include <err.h>

#include <jansson.h>
#include <reactor.h>

#include "flow.h"

static void usage(void)
{
  extern char *__progname;

  (void) fprintf(stderr, "usage: %s SPEC\n", __progname);
  exit(1);
}

int main(int argc, char **argv)
{
  json_t *spec;
  flow flow;

  if (argc != 2)
    usage();
  spec = json_load_file(argv[1], 0, NULL);
  if (!spec)
    err(1, "json_load_file");

  reactor_construct();
  signal(SIGINT, SIG_DFL);
  flow_construct(&flow, NULL, NULL);
  flow_configure(&flow, spec);
  if (flow.errors)
    err(1, "flow_configure");

  reactor_loop();

  flow_destruct(&flow);
  reactor_destruct();
  json_decref(spec);
}
