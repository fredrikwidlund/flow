#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>

#include "message.h"

static message_envelope *message_get_envelope(void *message)
{
  return (message_envelope *) ((uintptr_t) message - offsetof(message_envelope, message));
}

void *message_create(uintptr_t type, void *message, size_t size, void (*release)(void *))
{
  message_envelope *e;

  e = calloc(1, sizeof *e + size);
  *e = (message_envelope) {.ref = 1, .type = type, .size = size, .release = release};
  if (message)
    memcpy(e->message, message, size);
  return e->message;
}

void message_hold(void *message)
{
  message_envelope *e = message_get_envelope(message);

  e->ref++;
}

void message_release(void *message)
{
  message_envelope *e = message_get_envelope(message);

  e->ref--;
  if (e->ref == 0)
  {
    if (e->release)
      e->release(message);
    free(e);
  }
}

void message_send(int socket, void *message)
{
  ssize_t n;

  n = send(socket, &message, sizeof message, MSG_EOR);
  assert(n == sizeof message);
}

void *message_receive(int socket)
{
  ssize_t n;
  void *message;

  n = recv(socket, &message, sizeof message, 0);
  assert(n == sizeof message);

  return message;
}

uintptr_t message_type(void *message)
{
  message_envelope *e = message_get_envelope(message);

  return e->type;
}
