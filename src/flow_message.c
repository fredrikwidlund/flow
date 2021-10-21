#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <dynamic.h>

#include "flow.h"

typedef struct flow_envelope flow_envelope;

struct flow_envelope
{
  _Atomic int       references;
  int               type;
  const flow_table *table;
  char              message[];
};

static flow_envelope *flow_message_envelope(void *message)
{
  return (flow_envelope *) ((uintptr_t) message - offsetof(flow_envelope, message));
}

static void flow_table_destroy(void *message)
{
  (void) message;
}

static const flow_table flow_table_default = {.destroy = flow_table_destroy};

void *flow_message_create(void *message, size_t size, int type, const flow_table *table)
{
  flow_envelope *envelope;

  envelope = calloc(1, sizeof *envelope + size);
  *envelope = (flow_envelope) {.references = 1, .type = type, .table = table ? table : &flow_table_default};
  if (message)
    memcpy(envelope->message, message, size);
  return envelope->message;
}

void flow_message_hold(void *message)
{
  flow_envelope *envelope;

  if (message)
  {
    envelope = flow_message_envelope(message);
    __sync_add_and_fetch(&envelope->references, 1);
  }
}

void flow_message_release(void *message)
{
  flow_envelope *envelope;

  if (message)
  {
    envelope = flow_message_envelope(message);
    if (__sync_sub_and_fetch(&envelope->references, 1) == 0)
    {
      envelope->table->destroy(message);
    free(envelope);
    }
  }
}

int flow_message_type(void *message)
{
  return message ? flow_message_envelope(message)->type : 0;
}
