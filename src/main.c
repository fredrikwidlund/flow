#include <stdio.h>
#include <string.h>
#include <err.h>
#include <sys/stat.h>

#include <reactor.h>

#include "flow.h"

static json_t *load_configuration(const char *path)
{
  struct stat st;
  int e;
  FILE *f;
  char *data;
  json_t *spec;
  json_error_t error;
  string s;
  char *out, *value;
  ssize_t n, p1, p2;

  /* load file */
  e = stat(path, &st);
  if (e == -1)
    err(1, "stat");
  f = fopen(path, "r");
  if (!f)
    err(1, "fopen");
  data = malloc(st.st_size);
  n = fread(data, st.st_size, 1, f);
  if (n != 1)
    err(1, "fread");
  fclose(f);

  /* replace ${name} values with environment variable */
  string_construct(&s);
  string_insert(&s, 0, data);
  free(data);
  while (1)
  {
    p1 = string_find(&s, "${", 0);
    if (p1 == -1)
      break;
    p2 = string_find(&s, "}", p1);
    if (p2 == -1)
      break;
    string_data(&s)[p2] = 0;
    value = getenv(string_data(&s) + p1 + 2);
    string_replace(&s, p1, p2 + 1 - p1, value ? value : "");
  }
  out = strdup(string_data(&s));
  string_destruct(&s);

  /* load json */
  spec = json_loads(out, 0, &error);
  if (!spec)
    errx(1, "json_loads: %s", error.text);
  free(out);
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
  json_decref(spec);

  reactor_loop();

  flow_destruct(&flow);
  reactor_destruct();
}
