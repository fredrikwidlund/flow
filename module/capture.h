#ifndef CAPTURE_H_INCLUDED
#define CAPTURE_H_INCLUDED

typedef struct capture_frame capture_frame;
struct capture_frame
{
  uint64_t            time;
  struct sockaddr_ll *sll;
  segment             data;
};

#endif /* CAPTURE_H_INCLUDED */
