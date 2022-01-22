#ifndef LOG_H_INCLUDED
#define LOG_H_INCLUDED

#include "node.h"

#define log_critical(graph, format, ...) log_graph((graph), FLOW_CRIT, (format), ##__VA_ARGS__)

struct graph;

typedef struct log_event log_event;

struct log_event
{
  json_t *object;
};

void log_graph(struct graph *, int, const char *, ...);
void log_node(node *, int, json_t *);

#endif /* LOG_H_INCLUDED */
