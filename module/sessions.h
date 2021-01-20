#ifndef SESSIONS_H_INCLUDED
#define SESSIONS_H_INCLUDED

enum
{
  SESSIONS_ERROR = 0,
  SESSIONS_NEW
};

typedef struct session_endpoint session_endpoint;
typedef struct session          session;
typedef struct sessions         sessions;

struct session_endpoint
{
  uint32_t         ip;
  uint16_t         port;
};

struct session
{
  int              protocol;
  session_endpoint src;
  session_endpoint dst;
  size_t           sent;
  size_t           received;
  double           first_seen;
  double           last_seen;
};

struct sessions
{
  core_handler     user;
  int              debug;
  map              table;
  timer            timer;
};

extern int session_type;

void session_debug(session *);

#endif /* SESSIONS_H_INCLUDED */
