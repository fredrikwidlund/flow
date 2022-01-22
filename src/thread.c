#define _GNU_SOURCE

#include <assert.h>
#include <reactor.h>

#include "graph.h"
#include "queue.h"
#include "node.h"
#include "log.h"
#include "thread.h"

/* close event received from flow when flow_exit() is called */
static void thread_queue(reactor_event *event)
{
  assert(event->type == QUEUE_CLOSE);
  reactor_abort();
}

static void *thread_main(void *arg)
{
  thread *thread = arg;
  node *node;

  (void) pthread_setname_np(thread->id, thread->name);
  reactor_construct();

  list_foreach(&thread->graph->nodes, node)
    if (node->thread == thread)
      node_start(node);

  reactor_loop();
  /* loop should end due to all nodes explicitly being stopped */

  reactor_destruct();
  return NULL;
}

void thread_construct(thread *thread, struct graph *graph, const char *name)
{
  *thread = (struct thread) {.graph = graph, .name = name};
  queue_construct(&thread->queue, thread_queue, thread);
}

void thread_start(thread *thread)
{
  queue_start(&thread->queue);
  pthread_create(&thread->id, NULL, thread_main, thread);
}

void thread_stop(thread *thread)
{
  node *node;

  list_foreach(&thread->graph->nodes, node)
    if (node->thread == thread)
      queue_send(&node->queue, NULL);
  pthread_join(thread->id, NULL);
  queue_stop(&thread->queue);
}

void thread_destruct(thread *thread)
{
  queue_destruct(&thread->queue);
}
