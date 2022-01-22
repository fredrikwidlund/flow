#ifndef THREAD_H_INCLUDED
#define THREAD_H_INCLUDED

#include "queue.h"

struct graph;

typedef struct thread thread;

struct thread
{
  struct graph *graph;
  const char   *name;
  pthread_t     id;
  queue         queue;
};

void thread_construct(thread *, struct graph *, const char *);
void thread_start(thread *);
void thread_stop(thread *);
void thread_destruct(thread *);

#endif /* THREAD_H_INCLUDED */
