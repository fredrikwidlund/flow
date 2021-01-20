#include <assert.h>
#include <arpa/inet.h>

#include "netinet/ip.h"
#include "netinet/tcp.h"

#include <dynamic.h>
#include <reactor.h>
#include <jansson.h>

#include "flow.h"
#include "protocol.h"
#include "sessions.h"

int session_type = 0;

void session_debug(session *session)
{
  char src[INET_ADDRSTRLEN], dst[INET_ADDRSTRLEN];
  double d;

  d = (double) (session->last_seen - session->first_seen) / 1000000.0;
  (void) fprintf(stderr, "[%s:%d %s:%d %lu/%lu %.03f]\n", inet_ntop(AF_INET, &session->src.ip, src, sizeof src),
                 ntohs(session->src.port), inet_ntop(AF_INET, &session->dst.ip, dst, sizeof dst),
                 ntohs(session->dst.port), session->sent, session->received, d);
}

static size_t hash(void *p)
{
  session *s = p;
  uint64_t v = 0;

  v += s->src.ip + s->dst.ip;
  v <<= 16;
  v += s->src.port + s->dst.port;
  return hash_uint64(v);
}

static int endpoint_equal(session_endpoint a, session_endpoint b)
{
  return a.ip == b.ip && a.port == b.port;
}

static int match(session *a, session *b)
{
  return ((a->protocol == b->protocol) && ((endpoint_equal(a->src, b->src) && endpoint_equal(a->dst, b->dst)) ||
                                           (endpoint_equal(a->src, b->dst) && endpoint_equal(a->dst, b->src))));
}

static int equal(void *a, void *b)
{
  return b ? match(a, b) : ((session *) a)->protocol == PROTOCOL_NONE;
}
static void set(void *p1, void *p2)
{
  session *a = p1, *b = p2;

  *a = b ? *b : (session) {0};
}

static void tcp(sessions *sessions, struct iphdr *ip, struct tcphdr *tcp, size_t size)
{
  session session = {PROTOCOL_TCP, {ip->saddr, tcp->th_sport}, {ip->daddr, tcp->th_dport}, 0, 0, 0, 0}, *s;

  (void) size;
  s = map_at(&sessions->table, &session, hash, equal);

  if (s->protocol == PROTOCOL_NONE && tcp->th_flags & TH_SYN)
  {
    if (tcp->th_flags & TH_ACK)
    {
      session.src = (session_endpoint) {ip->daddr, tcp->th_dport};
      session.dst = (session_endpoint) {ip->saddr, tcp->th_sport};
    }
    session.first_seen = core_now(NULL);
    session.last_seen = core_now(NULL);
    map_insert(&sessions->table, &session, hash, set, equal, NULL);
    return;
  }

  if (s->protocol == PROTOCOL_TCP)
  {
    s->last_seen = core_now(NULL);
    if (endpoint_equal(s->src, session.src))
      s->sent += size;
    else
      s->received += size;
  }
}

static core_status event(core_event *event)
{
  sessions *sessions = event->state;
  protocol_headers *h = (protocol_headers *) event->data;

  (void) sessions;

  if (h->count >= 5 && h->header[2].type == PROTOCOL_IP && h->header[3].type == PROTOCOL_TCP &&
      h->header[4].type == PROTOCOL_DATA)
    tcp(sessions, h->header[2].ip, h->header[3].tcp, h->header[4].size);

  return CORE_OK;
}

static core_status timeout(core_event *event)
{
  sessions *sessions = event->state;
  session *session = sessions->table.elements;
  size_t i = 0;
  uint64_t t = core_now(NULL);
  core_status e;

  assert(event->type == TIMER_ALARM);
  while (i < sessions->table.elements_capacity)
  {
    if (session[i].protocol && t - session[i].last_seen > 5000000000)
    {
      if (sessions->debug)
        session_debug(&session[i]);
      e = core_dispatch(&sessions->user, session_type, (uintptr_t) &session[i]);
      if (e != CORE_OK)
        return e;
      map_erase(&sessions->table, &session[i], hash, set, equal, NULL);
      continue;
    }
    i++;
  }

  return CORE_OK;
}

static void *new (core_callback *callback, void *state, json_t *spec)
{
  sessions *sessions;

  sessions = malloc(sizeof *sessions);
  *sessions = (struct sessions) {.user = {.callback = callback, .state = state}};
  sessions->debug = json_is_true(json_object_get(spec, "debug"));
  timer_construct(&sessions->timer, timeout, sessions);
  map_construct(&sessions->table, sizeof(session), set);
  return sessions;
}

static void start(void *arg)
{
  sessions *sessions = arg;

  timer_set(&sessions->timer, 100000000, 100000000);
}

static void *load(void)
{
  session_type = flow_global_type();
  return NULL;
}

flow_table module_table = {.load = load, .new = new, .start = start, .event = event};
