#ifndef FLOW_H_INCLUDED
#define FLOW_H_INCLUDED

#include <ltdl.h>
#include <reactor.h>
#include <jansson.h>

typedef struct flow flow;
struct flow;

#include "flow_node.h"
#include "flow_log.h"
#include "flow_module.h"
#include "flow_message.h"

enum
{
  FLOW_ERROR    = 0,
  FLOW_EVENT
};

struct flow
{
  reactor_handler  handler;
  json_t       *graph;
  char         *name;
  int           debug;
  int           stats;
  timer         timer;

  flow_queue    events_receive;
  flow_queue    events_send;

  flow_modules  modules;
  flow_nodes    nodes;
  size_t        symbol_count;
  maps          symbols;
};

void  flow_construct(flow *, reactor_callback *, void *);
void  flow_destruct(flow *);
void  flow_open(flow *, json_t *);
void  flow_close(flow *);
void  flow_stats(flow *, int);
void  flow_search(flow *, const char *);
void  flow_load(flow *, const char *);
void  flow_register(flow *, const char *);
void  flow_add(flow *, const char *, json_t *);
void  flow_connect(flow *, const char *, const char *, json_t *);
void  flow_abort(flow_node *, const char *, ...);
void  flow_exit(flow_node *);
void *flow_create(void *, size_t, int, const flow_table *);
void  flow_send(flow_node *, void *);
void  flow_send_and_release(flow_node *, void *);
void  flow_hold(void *);
void  flow_release(void *);
int   flow_type(void *);
int   flow_symbol(flow *, const char *);


#endif /* FLOW_H_INCLUDED */
