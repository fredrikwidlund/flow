#ifndef FLOW_H_INCLUDED
#define FLOW_H_INCLUDED

typedef struct flow_module_handlers flow_module_handlers;
typedef struct flow_module          flow_module;
typedef struct flow_edge            flow_edge;
typedef struct flow_node            flow_node;
typedef struct flow                 flow;

struct flow_module_handlers
{
  void                 *(*load)(void);
  void                 *(*create)(flow_node *, json_t *);
  int                   (*start)(void *);
  void                  (*receive)(void *, void *);
  void                  (*destroy)(void *);
  void                  (*unload)(void *);
};

struct flow_module
{
  char                   *name;
  void                   *object;
  flow_module_handlers   *handlers;
  void                   *state;
};

struct flow_edge
{
  const char             *type;
  flow_node              *target;
};

struct flow_node
{
  flow_module            *module;
  const char             *id;
  list                    edges;
  void                   *state;
};

struct flow
{
  json_t                 *spec;
  list                    modules;
  list                    nodes;
};

void flow_construct(flow *);
void flow_destruct(flow *);
int  flow_load(flow *, char *);
int  flow_configure(flow *, json_t *);
void flow_send(flow_node *, void *);

#endif /* FLOW_H_INCLUDED */
