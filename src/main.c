#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <err.h>

#include <dynamic.h>
#include <reactor.h>
#include <jansson.h>

#include "flow.h"

static void usage(void)
{
  extern char *__progname;

  (void) fprintf(stderr, "usage: %s SPEC\n", __progname);
  exit(1);
}

int main(int argc, char **argv)
{
  flow f;
  int e;

  if (argc != 2)
    usage();

  core_construct(NULL);
  flow_construct(&f);
  e = flow_load(&f, argv[1]);
  if (e == -1)
    err(1, "flow_load");

  core_loop(NULL);

  flow_destruct(&f);
  core_destruct(NULL);
}
