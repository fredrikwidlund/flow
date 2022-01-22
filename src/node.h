#ifndef NODE_H_INCLUDED
#define NODE_H_INCLUDED

#include "modules.h"
#include "queue.h"
#include "thread.h"
#include "edge.h"

typedef struct node node;

struct node
{
  struct graph *graph;
  thread       *thread;
  queue         queue;
  const char   *name;
  const char   *group;
  module       *module;
  json_t       *spec;
  list          edges;
  void         *state;
};

void  node_construct(node *, struct graph *, const char *, const char *, module *, thread *, json_t *);
void  node_add(node *, node *, json_t *);
void  node_start(node *);
void  node_stop(node *);
void  node_destruct(node *);
int   node_lookup(node *, const char *);
void  node_send(node *, void *);
void  node_exit(node *);

#endif /* NODE_H_INLCUDED */
