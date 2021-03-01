#include <stdio.h>
#include <signal.h>
#include <err.h>

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
  flow flow;

  if (argc != 2)
    usage();

  reactor_construct();
  flow_construct(&flow, NULL, NULL);
  flow_configure(&flow, argv[1]);

  reactor_loop();

  flow_destruct(&flow);
  reactor_destruct();
}
