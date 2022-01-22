#include <stdio.h>
#include <assert.h>

#include <reactor.h>
#include <flow.h>

static void flow_event(reactor_event *event)
{
  flow_log_event *message = (flow_log_event *) event->data;

  assert(event->type == FLOW_LOG);
  (void) json_dumpf(message->object, stderr, JSON_COMPACT | JSON_REAL_PRECISION(6));
  (void) fprintf(stderr, "\n");
}

int main(int argc, char **argv)
{
  char *conf = argc >= 2 ? argv[1] : "flow.json";
  flow flow;

  reactor_construct();
  flow_construct(&flow, flow_event, NULL);
  flow_load(&flow, conf);

  reactor_loop();

  flow_destruct(&flow);
  reactor_destruct();
}
