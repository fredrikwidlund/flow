#ifndef FLOW_H_INCLUDED
#define FLOW_H_INCLUDED

#include <ltdl.h>

typedef struct flow_node   flow_node;
typedef struct flow_table  flow_table;
typedef struct flow_module flow_module;
typedef struct flow        flow;

// simplify functions...? (see statistics)
// load()
// unload*(
// new()
// configure()
// start()
// receive()
// stop()
// free()

struct flow_table
{
  void         *(*load)(void);
  void          (*unload)(void *);
  void         *(*new)(core_callback *, void *, json_t *);
  void          (*release)(void *);
  void          (*start)(void *);
  core_status   (*event)(core_event *);
};

struct flow_node
{
  flow_module  *module;
  char         *name;
  void         *state;
  list          edges;
};

struct flow_module
{
  char         *name;
  lt_dlhandle   handle;
  flow_table   *table;
  void          *state;
};

struct flow
{
  core_handler  user;
  size_t        errors;
  list          modules;
  list          nodes;
};

void flow_construct(flow *, core_callback *, void *);
void flow_configure(flow *, json_t *);
void flow_destruct(flow *);
void flow_send(flow_node *, void *);
int  flow_global_type(void);

#endif /* FLOW_H_INCLUDED */
