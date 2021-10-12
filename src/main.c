#include <stdio.h>
#include <signal.h>
#include <err.h>

#include <reactor.h>

#include "flow.h"

static json_t *load_configuration(char *path)
{
  json_t *spec;
  json_error_t error;

  spec = json_load_file(path, 0, &error);
  if (!spec)
    warnx("json_load_file: %s", error.text);
  return spec;
}

static core_status flow_event(core_event *event)
{
  flow *flow = event->state;
  flow_log_message *message;
  json_t *stats, *o;
  int i;
  const char *name;

  switch (event->type)
  {
  case FLOW_STATS:
    stats = (json_t *) event->data;
    flockfile(stdout);
    fprintf(stdout, "[stats]");
    i = 0;
    json_object_foreach(stats, name, o)
    {
      fprintf(stdout, "%s%s %lld/%lld", i ? ", " : " ", name,
              json_integer_value(json_object_get(o, "received")),
              json_integer_value(json_object_get(o, "sent")));
      i++;
    }
    fprintf(stdout, "\n");
    funlockfile(stdout);
    break;
  case FLOW_LOG:
    message = (flow_log_message *) event->data;
    if (message->level == FLOW_DEBUG && !flow->debug)
      break;
    flockfile(stdout);
    fprintf(stdout, "[%s] %s\n", message->severity, message->description);
    funlockfile(stdout);
    break;
  }
  return CORE_OK;
}

int main(int argc, char **argv)
{
  flow flow;
  json_t *spec;

  reactor_construct();
  flow_construct(&flow, flow_event, &flow);

  spec = load_configuration(argc >= 2 ? argv[1] : "flow.json");
  flow_open(&flow, spec);

  reactor_loop();

  flow_destruct(&flow);
  reactor_destruct();
  json_decref(spec);
}
