#ifndef FLOW_LOG_H_INCLUDED
#define FLOW_LOG_H_INCLUDED

typedef enum flow_log_level     flow_log_level;
typedef struct flow_log_message flow_log_message;

enum
{
  FLOW_ERROR    = 0,
  FLOW_LOG
};

enum flow_log_level
{
  FLOW_CRITICAL = 2,
  FLOW_INFO     = 6,
  FLOW_DEBUG    = 7
};

struct flow_log_message
{
  flow_log_level  level;
  const char     *severity;
  char           *description;
};

void flow_log(flow *, int, const char *, ...);

#endif /* FLOW_LOG_H_INCLUDED */
