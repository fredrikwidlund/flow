#ifndef QUEUE_H_INCLUDED
#define QUEUE_H_INCLUDED

#include <reactor.h>

enum
{
  QUEUE_CLOSE,
  QUEUE_MESSAGE
};

typedef struct queue queue;

struct queue
{
  reactor_handler user;
  reactor_handler reactor_handler;
  int             fd[2];
};

void queue_construct(queue *, reactor_callback *, void *);
void queue_start(queue *);
void queue_send(queue *, void *);
void queue_read(queue *);
void queue_stop(queue *);
void queue_destruct(queue *);

#endif /* QUEUE_H_INCLUDED */
