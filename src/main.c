#include <stdio.h>
#include <string.h>
#include <err.h>
#include <assert.h>
#include <sys/stat.h>

#include <reactor.h>

#include "flow.h"

data data_merge(data d1, data d2)
{
  return data_construct(data_base(d1), data_offset(d1, d2) + data_size(d2));
}

static json_t *load_configuration(const char *path)
{
  string s;
  data prefix, postfix;
  char *value;
  json_t *spec;
  json_error_t error;

  s = string_load(path);
  while (1)
  {
    prefix = string_find_data(s, data_string("${"));
    if (data_empty(prefix))
      break;
    postfix = string_find_at_data(s, data_offset(string_data(s), prefix) + data_size(prefix), data_string("}"));
    if (data_empty(postfix))
      break;
    ((char *) data_base(postfix))[0] = 0;
    value = getenv((char *) data_base(prefix) + data_size(prefix));
    if (value)
      s = string_replace_data(s, data_merge(prefix, postfix), data_string(value));
  }

  spec = json_loads(s, 0, &error);
  if (!spec)
    errx(1, "json_loads: %s", error.text);
  string_free(s);
  return spec;
}

static void flow_event(reactor_event *event)
{
  flow_log_event *log;
  char *line;

  switch (event->type)
  {
  case FLOW_EVENT:
    log = (flow_log_event *) event->data;
    line = json_dumps(log->event, JSON_COMPACT | JSON_REAL_PRECISION(6));
    (void) fprintf(stdout, "%s\n", line);
    (void) fflush(stdout);
    free(line);
    break;
  }
}

int main(int argc, char **argv)
{
  flow flow;
  json_t *spec;
  char *s;

  spec = load_configuration(argc >= 2 ? argv[1] : "flow.json");
  s = json_dumps(spec, JSON_INDENT(2));
  printf("spec: %s\n", s);
  free(s);
  json_decref(spec);
  return 0;

  reactor_construct();
  flow_construct(&flow, flow_event, &flow);


  flow_open(&flow, spec);
  json_decref(spec);

  reactor_loop();

  flow_destruct(&flow);
  reactor_destruct();
}
