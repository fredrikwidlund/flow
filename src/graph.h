#ifndef GRAPH_H_INCLUDED
#define GRAPH_H_INCLUDED

#include <jansson.h>

#include "symbols.h"
#include "modules.h"
#include "node.h"
#include "queue.h"
#include "thread.h"

typedef struct graph graph;

struct graph
{
  reactor_handler  user;
  json_t          *spec;
  symbols          symbols;
  modules          modules;
  queue            events;
  list             threads;
  list             nodes;
};

void graph_construct(graph *, reactor_callback *, void *);
void graph_load(graph *, const char *);
void graph_open(graph *, json_t *);
void graph_register(graph *, const char *);
void graph_stop(graph *);
void graph_destruct(graph *);

#endif /* GRAPH_H_INCLUDED */
