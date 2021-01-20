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
#include "portscan.h"

int portscan_type = 0;

static void debug(portscan_node *node)
{
  char ip[32];
  uint32_t addr;
  mapi_entry *e;

  addr = ntohl(node->network);
  inet_ntop(AF_INET, &addr, ip, sizeof ip);
  (void) fprintf(stderr, "[%s/24 reported]\n", ip);
  mapi_foreach(&node->peers, e)
  {
    addr = htonl(e->key >> 16);
    inet_ntop(AF_INET, &addr, ip, sizeof ip);
    (void) fprintf(stderr, "    %s:%lu\n", ip, e->key & 0xffff);
  }
}

static core_status report(portscan *portscan, portscan_node *node)
{
  json_t *object, *array;
  uint32_t addr;
  char ip[32], endpoint[32];
  mapi_entry *e;
  core_status status;

  object = json_object();
  json_object_set_new(object, "type", json_string("portscan"));

  addr = ntohl(node->address);
  inet_ntop(AF_INET, &addr, ip, sizeof ip);
  json_object_set_new(object, "address", json_string(ip));

  addr = ntohl(node->network);
  inet_ntop(AF_INET, &addr, ip, sizeof ip);
  json_object_set_new(object, "network", json_string(ip));

  array = json_array();
  mapi_foreach(&node->peers, e)
  {
    addr = htonl(e->key >> 16);
    inet_ntop(AF_INET, &addr, ip, sizeof ip);
    snprintf(endpoint, sizeof endpoint, "%s:%lu", ip, e->key & 0xffff);
    json_array_append_new(array, json_string(endpoint));
  }
  json_object_set_new(object, "targets", array);

  status = core_dispatch(&portscan->user, portscan_type, (uintptr_t) object);
  json_decref(object);
  return status;
}

static core_status event(core_event *event)
{
  portscan *portscan = event->state;
  portscan_node *node;
  session *session = (struct session *) event->data;
  uint64_t node_id, peer_id;

  if (session->sent || session->received)
    return CORE_OK;

  node_id = ntohl(session->src.ip) & 0xffffff00;
  if (node_id == 0xc20fd400)
    return CORE_OK;

  peer_id = ((uint64_t) ntohl(session->dst.ip)) << 16 | ntohs(session->dst.port);

  node = (portscan_node *) mapi_at(&portscan->nodes, node_id);
  if (!node)
  {
    node = malloc(sizeof *node);
    node->network = node_id;
    node->address = ntohl(session->src.ip);
    node->peer_limit_reached = 0;
    node->peer_count = 0;
    mapi_construct(&node->peers);
    mapi_insert(&portscan->nodes, node_id, (uintptr_t) node, NULL);
  }

  if (node->peer_limit_reached || mapi_at(&node->peers, peer_id))
    return CORE_OK;

  mapi_insert(&node->peers, peer_id, peer_id, NULL);
  node->peer_count++;
  if (node->peer_count >= 16)
  {
    node->peer_limit_reached = 1;
    if (portscan->debug)
      debug(node);
    report(portscan, node);
  }

  return CORE_OK;
}

static void *new (core_callback *callback, void *state, json_t *spec)
{
  portscan *portscan;

  portscan = malloc(sizeof *portscan);
  *portscan = (struct portscan) {.user = {.callback = callback, .state = state}};
  portscan->debug = json_is_true(json_object_get(spec, "debug"));
  mapi_construct(&portscan->nodes);
  return portscan;
}

static void *load(void)
{
  portscan_type = flow_global_type();
  return NULL;
}

flow_table module_table = {.load = load, .new = new, .event = event};
