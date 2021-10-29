#ifndef FLOW_LOG_H_INCLUDED
#define FLOW_LOG_H_INCLUDED

enum flow_log_level
{
  FLOW_LOG_EMERG  = 0,
  FLOW_LOG_ALERT  = 1,
  FLOW_LOG_CRIT   = 2,
  FLOW_LOG_ERROR  = 3,
  FLOW_LOG_WARN   = 4,
  FLOW_LOG_NOTICE = 5,
  FLOW_LOG_INFO   = 6,
  FLOW_LOG_DEBUG  = 7
};

typedef enum flow_log_level     flow_log_level;
typedef struct flow_log_event   flow_log_event;

struct flow_log_event
{
  json_t         *event;
};

flow_log_event *flow_log_create(const char *, flow_log_level, const char *, json_t *);
void            flow_log(flow_node *, flow_log_level, const char *, json_t *);
void            flow_log_message(flow_node *, flow_log_level, const char *, ...);
void            flow_log_sync(flow *, flow_log_level, const char *, json_t *);
void            flow_log_sync_message(flow *, flow_log_level, const char *, ...);

#endif /* FLOW_LOG_H_INCLUDED */
