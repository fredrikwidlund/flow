#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <reactor.h>

#include "flow_queue.h"

static int flow_queue_read(flow_queue *queue, void **message)
{
  ssize_t n;

  n = recv(queue->socket, message, sizeof *message, 0);
  if (n == 0 || (n == -1 && errno == EAGAIN))
    return 0;
  assert(n == sizeof message);
  return 1;
}

static void flow_queue_receive(reactor_event *event)
{
  flow_queue *queue = event->state;
  void *message;
  int n, cancel = 0;

  queue->cancel = &cancel;
  while (!cancel)
  {
    n = flow_queue_read(queue, &message);
    if (!n)
      break;
    reactor_dispatch(&queue->handler, message ? FLOW_QUEUE_MESSAGE : FLOW_QUEUE_END, (uintptr_t) message);
  }
  queue->cancel = NULL;
}

void flow_queue_construct(flow_queue *queue, reactor_callback *callback, void *state)
{
  *queue = (flow_queue) {.socket = -1};

  reactor_handler_construct(&queue->handler, callback, state);
  descriptor_construct(&queue->descriptor, flow_queue_receive, queue);
}

void flow_queue_pair(flow_queue *queue1, flow_queue *queue2)
{
  int fd[2], e;

  e = socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_NONBLOCK, PF_UNSPEC, fd);
  assert(e == 0);
  queue1->socket = fd[0];
  queue2->socket = fd[1];
}

void flow_queue_open(flow_queue *queue)
{
  descriptor_open(&queue->descriptor, queue->socket, DESCRIPTOR_READ);
}

void flow_queue_send(flow_queue *queue, void *message)
{
  ssize_t n;

  flow_message_hold(message);
  n = send(queue->socket, &message, sizeof message, MSG_EOR);
  assert(n == sizeof message);
}

static void flow_queue_flush(flow_queue *queue)
{
  void *message;
  int n;

  while (1)
  {
    n = flow_queue_read(queue, &message);
    if (!n)
      break;
    if (message)
      flow_message_release(message);
  }
}

void flow_queue_close(flow_queue *queue)
{
  if (descriptor_is_open(&queue->descriptor))
  {
    if (queue->cancel)
      *(queue->cancel) = 1;
    flow_queue_flush(queue);
    descriptor_close(&queue->descriptor);
  }
}

void flow_queue_destruct(flow_queue *queue)
{
  flow_queue_close(queue);
}
