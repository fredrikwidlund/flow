#include "edge.h"

void edge_construct(edge *edge, node *target, int type)
{
  *edge = (struct edge) {.target = target, .type = type};
}
