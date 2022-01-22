#ifndef FLOW_H_INCLUDED
#define FLOW_H_INCLUDED

#include <jansson.h>
#include <reactor.h>

/* main api */

enum flow_event_type
{
  FLOW_LOG
};

enum flow_log_level
{
  FLOW_CRIT  = 2,
  FLOW_INFO  = 6,
  FLOW_DEBUG = 7
};

typedef struct flow_log_event flow_log_event;
typedef struct flow           flow;

struct flow
{
  void *internal;
};

struct flow_log_event
{
  json_t *object;
};

void flow_construct(flow *, reactor_callback *, void *);
void flow_destruct(flow *);
void flow_load(flow *, const char *);

/* module api */

void flow_register(void *, const char *);

/* message api */

typedef struct flow_message_table flow_message_table;

struct flow_message_table
{
  void  (*destroy)(void *);
};

void *flow_create(void *, size_t, int, const flow_message_table *);
void  flow_hold(void *);
void  flow_release(void *);

/* flow module */

typedef struct flow_module_table flow_module_table;

struct flow_module_table
{
  void *(*load)(void *);
  void  (*unload)(void *);
  void *(*create)(void *, json_t *);
  void  (*destroy)(void *);
  void  (*receive)(void *, void *);
};

/* node api */

int  flow_lookup(void *, const char *);
void flow_log(void *, int, json_t *);
void flow_log_message(void *, int, const char *, ...);
void flow_send(void *, void *);
void flow_send_and_release(void *, void *);
void flow_exit(void *);

#endif /* FLOW_H_INCLUDED */
