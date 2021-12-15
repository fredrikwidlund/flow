#ifndef FLOW_QUEUE_H_INCLUDED
#define FLOW_QUEUE_H_INCLUDED

#include "flow_message.h"

enum
{
  FLOW_QUEUE_MESSAGE,
  FLOW_QUEUE_END
};

typedef struct flow_queue flow_queue;

struct flow_queue
{
  reactor_handler handler;
  reactor_handler socket_handler;
  int             socket;
  int             cancel;
};

void flow_queue_construct(flow_queue *, flow_queue *);
void flow_queue_listen(flow_queue *, reactor_callback *, void *);
void flow_queue_send(flow_queue *, void *);
void flow_queue_unlisten(flow_queue *);
void flow_queue_destruct(flow_queue *);

#endif /* FLOW_QUEUE_H_INCLUDED */
