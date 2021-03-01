#ifndef FLOW_H_INCLUDED
#define FLOW_H_INCLUDED

#include <ltdl.h>
#include <jansson.h>

enum
{
  FLOW_DEBUG = 0x01
};

typedef struct flow_node   flow_node;
typedef struct flow_table  flow_table;
typedef struct flow_stats  flow_stats;
typedef struct flow_module flow_module;
typedef struct flow        flow;

struct flow_table
{
  void         *(*load)(void);
  void          (*unload)(void *);
  void         *(*constructor)(json_t *, int);
  void          (*destructor)(void *);
  void          (*receive)(void *, void *);
};

struct flow_stats
{
  size_t        sent;
  size_t        received;
};

struct flow_node
{
  flow         *flow;
  flow_module  *module;
  const char   *name;
  json_t       *spec;
  int           sockets[2];
  pthread_t     thread;
  void         *state;
  list          edges;
  flow_stats    stats;
};

struct flow_module
{
  const char   *name;
  lt_dlhandle   handle;
  flow_table   *table;
  void         *state;
};

struct flow
{
  core_handler  user;
  json_t       *spec;
  int           flags;
  size_t        errors;
  list          modules;
  list          nodes;
  timer         timer;
};

void flow_construct(flow *, core_callback *, void *);
void flow_configure(flow *, char *);
void flow_destruct(flow *);
void flow_send(flow_node *, void *);
int  flow_global_type(void);

#endif /* FLOW_H_INCLUDED */
