#ifndef FLOW_MESSAGE_H_INCLUDED
#define FLOW_MESSAGE_H_INCLUDED

typedef struct flow_table flow_table;

struct flow_table
{
  void (*destroy)(void *);
};

void *flow_message_create(void *, size_t, int, const flow_table *);
void  flow_message_hold(void *);
void  flow_message_release(void *);
int   flow_message_type(void *);

#endif /* FLOW_MESSAGE_H_INCLUDED */
