#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <errno.h>
#include <assert.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <dynamic.h>

#include "flow_queue.h"

static int flow_queue_read(flow_queue *queue, void **message)
{
  ssize_t n;

  assert(queue->active);
  n = recv(queue->socket, message, sizeof *message, 0);
  if (n == 0 || (n == -1 && errno == EAGAIN))
    return 0;
  assert(n == sizeof message);
  return 1;
}

static core_status flow_queue_receive(core_event *event)
{
  flow_queue *queue = event->state;
  void *message;
  int n, cancel = 0;
  core_status e;

  queue->cancel = &cancel;
  while (1)
  {
    n = flow_queue_read(queue, &message);
    if (!n)
      break;
    reactor_dispatch(&queue->handler, message ? FLOW_QUEUE_MESSAGE : FLOW_QUEUE_END, (uintptr_t) message);
    if (cancel)
      return;
  }
  queue->cancel = NULL;
}

void flow_queue_construct(flow_queue *head, flow_queue *tail)
{
  int fd[2];

  *head = (flow_queue) {.socket = fd[0], .active = 1};
  *tail = (flow_queue) {.socket = fd[1], .active = 1};
}

void flow_queue_listen(flow_queue *queue, reactor_callback *callback, void *state)
{
  int e;

  e = fcntl(queue->socket, F_SETFL, O_NONBLOCK);
  assert(e == 0);
  reactor_handler_construct(&queue->handler, callback, state);
  reactor_handler_construct(&queue->socket_handler, flow_queue_receive, queue);
  reactor_add(&queue->socket_handler, queue->socket, EPOLLIN | EPOLLET);
}

void flow_queue_send(flow_queue *queue, void *message)
{
  ssize_t n;

  flow_message_hold(message);
  n = send(queue->socket, &message, sizeof message, MSG_EOR);
  assert(n == sizeof message);
}

void flow_queue_unlisten(flow_queue *queue)
{
  reactor_delete(&queue->socket_handler, queue->socket);
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

void flow_queue_destruct(flow_queue *queue)
{
  if (queue->cancel)
    *(queue->cancel) = 0;
  flow_queue_flush(queue);
  flow_queue_unlisten(queue);
}
}
