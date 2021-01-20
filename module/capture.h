#ifndef CAPTURE_H_INCLUDED
#define CAPTURE_H_INCLUDED

enum
{
  CAPTURE_ERROR,
  CAPTURE_FRAME
};

typedef struct capture_frame capture_frame;
typedef struct capture       capture;

struct capture_frame
{
  uint64_t            time;
  struct sockaddr_ll *sll;
  segment             data;
};

struct capture
{
  core_handler        user;
  json_t             *spec;
  int                 interface;
  int                 fd;
  int                 block;
  void               *data;
};

#endif /* CAPTURE_H_INCLUDED */
