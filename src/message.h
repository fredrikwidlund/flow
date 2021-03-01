
#ifndef MESSAGE_H_INCLUDED
#define MESSAGE_H_INCLUDED

typedef struct message_envelope message_envelope;
struct message_envelope
{
  size_t      ref;
  uintptr_t   type;
  size_t      size;
  void      (*release)(void *);
  char        message[];
};

void       message_set_socket(int);
void       message_hold(void *);
void       message_release(void *);
void      *message_create(uintptr_t, void *, size_t, void (*)(void *));
void       message_send(int, void *);
void      *message_receive(int);
uintptr_t  message_type(void *);

#endif /* MESSAGE_H_INCLUDED */

