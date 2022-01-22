#include <string.h>

#include "message.h"

typedef struct envelope envelope;

struct envelope
{
  _Atomic int          references;
  int                  type;
  const message_table *table;
  char                 message[];
};

static envelope *message_envelope(void *message)
{
  return (envelope *) ((uintptr_t) message - offsetof(envelope, message));
}

static void message_destroy_default(void *message)
{
}

static const message_table message_table_default = {.destroy = message_destroy_default};

void *message_create(void *message, size_t size, int type, const message_table *table)
{
  envelope *envelope = malloc(sizeof *envelope + size);

  *envelope = (struct envelope) {.references = 1, .type = type, .table = table ? table : &message_table_default};
  if (message)
    memcpy(envelope->message, message, size);
  return envelope->message;
}

int message_type(void *message)
{
  envelope *envelope = message_envelope(message);

  return envelope->type;
}

void message_hold(void *message)
{
  envelope *envelope = message_envelope(message);

  __sync_add_and_fetch(&envelope->references, 1);
}

void message_release(void *message)
{
  envelope *envelope = message_envelope(message);

  if (__sync_sub_and_fetch(&envelope->references, 1) == 0)
  {
    envelope->table->destroy(message);
    free(envelope);
  }
}
