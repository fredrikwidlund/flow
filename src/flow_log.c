#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <jansson.h>

#include "flow.h"
#include "flow_log.h"

void flow_log(flow *flow, int level, const char *format, ...)
{
  const char *severities[] = {"emergency", "alert", "critical", "error", "warning", "notice", "information", "debug"};
  flow_log_message message = {.level = level};
  va_list ap;

  if (level < 0 || level > FLOW_DEBUG)
    flow_log(flow, FLOW_CRITICAL, "invalid log level");
  message.severity = severities[level];
  va_start(ap, format);
  assert(vasprintf(&message.description, format, ap) != -1);
  core_dispatch(&flow->user, FLOW_LOG, (uintptr_t) &message);
  va_end(ap);
  free(message.description);
  if (level <= FLOW_CRITICAL)
  {
    core_dispatch(&flow->user, FLOW_ERROR, 0);
    exit(1);
  }
}
