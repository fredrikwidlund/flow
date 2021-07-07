#include <stdio.h>
#include <signal.h>
#include <err.h>

#include <reactor.h>

#include "flow.h"
#include "flow_log.h"

static core_status flow_event(core_event *event)
{
  flow_log_message *message = (flow_log_message *) event->data;

  switch (event->type)
  {
  case FLOW_LOG:
    flockfile(stdout);
    fprintf(stdout, "[%s] %s\n", message->severity, message->description);
    funlockfile(stdout);
    break;
  }
  return CORE_OK;
}

int main()
{
  flow flow;

  reactor_construct();
  flow_construct(&flow, flow_event, &flow);

  flow_search(&flow, ".");
  flow_register(&flow, "sender");
  flow_register(&flow, "receiver");
  flow_add(&flow, "sender0", NULL);
  flow_add(&flow, "receiver0", NULL);
  flow_add(&flow, "receiver1", NULL);
  flow_add(&flow, "receiver2", NULL);

  flow_connect(&flow, "sender0", "receiver0");
  flow_connect(&flow, "sender0", "receiver1");
  flow_connect(&flow, "sender0", "receiver2");

  reactor_loop();

  flow_destruct(&flow);
  reactor_destruct();
}
