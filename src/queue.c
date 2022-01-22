#define _GNU_SOURCE

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include <reactor.h>

#include "queue.h"
#include "message.h"

void queue_read(queue *queue)
{
  void *message[1024];
  ssize_t n, i;

  n = read(queue->fd[0], message, sizeof message);
  if (n == -1 && errno == EAGAIN)
    return;

  assert(n % sizeof message[0] == 0);
  n /= sizeof message[0];

  /* forward messages */
  for (i = 0; i < n; i ++)
  {
    if (!message[i])
    {
      reactor_dispatch(&queue->user, QUEUE_CLOSE, 0);
      break;
    }

    reactor_dispatch(&queue->user, QUEUE_MESSAGE, (uintptr_t) message[i]);
    message_release(message[i]);
  }

    /* flush remaining */
  /*
  i ++;
  for (; i < n; i ++)
    if (message[i])
      message_release(message[i]);
  */


}

static void queue_event(reactor_event *reactor_event)
{
  struct epoll_event *event = (struct epoll_event *) reactor_event->data;

  assert(event->events == EPOLLIN);
  queue_read(reactor_event->state);
}

void queue_flush(queue *queue)
{
  ssize_t n;
  void *message;

  while (1)
  {
    n = read(queue->fd[0], &message, sizeof message);
    if (n != sizeof message)
      break;
    if (message)
      message_release(message);
  }
}

void queue_construct(queue *queue, reactor_callback *callback, void *state)
{
  *queue = (struct queue) {.user = {.callback = callback ,.state = state}};

  assert(pipe(queue->fd) == 0);
  assert(fcntl(queue->fd[0], F_SETFL, O_NONBLOCK) == 0);
  assert(fcntl(queue->fd[1], F_SETFL, O_NONBLOCK) == 0);
  assert(fcntl(queue->fd[1], F_SETPIPE_SZ, 1048576) == 1048576);
}

void queue_start(queue *queue)
{
  reactor_handler_construct(&queue->reactor_handler, queue_event, queue);
  reactor_add(&queue->reactor_handler, queue->fd[0], EPOLLIN);
}

void queue_send(queue *queue, void *message)
{
  if (message)
    message_hold(message);
  assert(write(queue->fd[1], &message, sizeof message) == sizeof message);
}

void queue_stop(queue *queue)
{
  reactor_delete(NULL, queue->fd[0]);
}

void queue_destruct(queue *queue)
{
  queue_flush(queue);
  (void) close(queue->fd[0]);
  (void) close(queue->fd[1]);
}
