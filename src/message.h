#ifndef MESSAGE_H_INCLUDED
#define MESSAGE_H_INCLUDED

#include "flow.h"

typedef flow_message_table message_table;

void *message_create(void *, size_t, int, const message_table *);
void  message_hold(void *);
void  message_release(void *);
int   message_type(void *);

#endif /* MESSAGE_H_INCLUDED */
