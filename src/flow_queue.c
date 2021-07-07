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
  int n;
  core_status e;

  assert(queue->active && queue->listening);
  while (1)
  {
    n = flow_queue_read(queue, &message);
    if (!n)
      return CORE_OK;
    if (!message)
      return core_dispatch(&queue->user, FLOW_QUEUE_END, 0);

    e = core_dispatch(&queue->user, FLOW_QUEUE_MESSAGE, (uintptr_t) message);
    if (e != CORE_OK)
      return e;
  }
}

void flow_queue_construct(flow_queue *head, flow_queue *tail)
{
  int fd[2];

  assert(socketpair(AF_UNIX, SOCK_SEQPACKET, PF_UNSPEC, fd) == 0);
  *head = (flow_queue) {.socket = fd[0], .active = 1};
  *tail = (flow_queue) {.socket = fd[1], .active = 1};
}

void flow_queue_listen(flow_queue *queue, core_callback *callback, void *state)
{
  assert(queue->active && !queue->listening);
  assert(fcntl(queue->socket, F_SETFL, O_NONBLOCK) == 0);
  queue->user = (core_handler) {.callback = callback, .state = state};
  core_add(NULL, flow_queue_receive, queue, queue->socket, EPOLLIN | EPOLLET);
  queue->listening = 1;
}

void flow_queue_send(flow_queue *queue, void *message)
{
  assert(queue->active);
  flow_message_hold(message);
  assert(send(queue->socket, &message, sizeof message, MSG_EOR) == sizeof message);
}

void flow_queue_unlisten(flow_queue *queue)
{
  assert(queue->active && queue->listening);
  core_delete(NULL, queue->socket);
  queue->listening = 0;
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
  if (queue->active)
  {
    flow_queue_flush(queue);
    if (queue->listening)
      flow_queue_unlisten(queue);
    assert(close(queue->socket) == 0);
    *queue = (flow_queue) {0};
  }
}
