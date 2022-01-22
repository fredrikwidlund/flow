#ifndef EDGE_H_INCLUDED
#define EDGE_H_INCLUDED

#include "node.h"

struct node;

typedef struct edge edge;

struct edge
{
  struct node *target;
  int          type;
};

void edge_construct(edge *, struct node *, int);

#endif /* EDGE_H_INCLUDED */
