#ifndef FLOW_NODE_H_INCLUDED
#define FLOW_NODE_H_INCLUDED

#include "flow_module.h"
#include "flow_queue.h"

typedef struct flow_node flow_node;
typedef struct flow_edge flow_edge;
typedef list             flow_nodes;

struct flow_node
{
  flow              *flow;
  char              *name;
  json_t            *metadata;
  int                active;
  int                detached;
  flow_module       *module;
  flow_queue         head;
  flow_queue         tail;
  pthread_t          thread;
  void              *state;
  list               edges;
  size_t             detached_neighbours;
  size_t             attached_neighbours;
  _Atomic size_t     received;
  _Atomic size_t     sent;
};

struct flow_edge
{
  flow_node         *target;
  int                type;
  json_t            *spec;
};

void flow_nodes_construct(flow *);
void flow_nodes_add(flow *, const char *, json_t *);
void flow_nodes_connect(flow *, const char *, const char *, json_t *);
void flow_nodes_destruct(flow *);
void flow_nodes_stop(flow *);

void flow_node_send(flow_node *, void *);
void flow_node_exit(flow_node *);

#endif /* FLOW_NODE_H_INCLUDED */
