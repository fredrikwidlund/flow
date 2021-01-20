#include <dynamic.h>
#include <reactor.h>
#include <jansson.h>
#include <GeoIP.h>
#include <GeoIPCity.h>

#include "portscan.h"
#include "sessions.h"
#include "flow.h"

typedef struct offender offender;
typedef struct aggregate aggregate;

struct offender
{
  uint32_t     address;
  json_t      *portscan;
  int          attack;
};

struct aggregate
{
  core_handler user;
  mapi         offenders;
};

static void portscan_event(aggregate *aggregate, json_t *object)
{
  uint32_t address;
  offender *offender;

  inet_pton(AF_INET, json_string_value(json_object_get(object, "address")), &address);
  address = ntohl(address);
  offender = (struct offender *) mapi_at(&aggregate->offenders, address & 0xffffff00);
  if (!offender)
  {
    offender = malloc(sizeof *offender);
    *offender = (struct offender) {.address = address};
    offender->portscan = object;
    json_incref(offender->portscan);
    mapi_insert(&aggregate->offenders, address & 0xffffff00, (uintptr_t) offender, NULL);
  }
}

static void session_event(aggregate *aggregate, session *session)
{
  uint32_t address, network;
  offender *offender;
  json_t *object;
  char ip[32], endpoint[32];

  address = ntohl(session->src.ip);
  network = address & 0xffffff00;
  offender = (struct offender *) mapi_at(&aggregate->offenders, network);
  if (offender && offender->portscan && (session->sent || session->received))
  {
    offender->attack++;
    object = json_object();
    json_object_set_new(object, "type", json_string("offender"));

    address = htonl(address);
    inet_ntop(AF_INET, &address, ip, sizeof ip);
    json_object_set_new(object, "address", json_string(ip));

    network = htonl(network);
    inet_ntop(AF_INET, &network, ip, sizeof ip);
    json_object_set_new(object, "network", json_string(ip));

    json_object_set_new(object, "attacks", json_integer(offender->attack));
    inet_ntop(AF_INET, &session->dst.ip, ip, sizeof ip);
    snprintf(endpoint, sizeof endpoint, "%s:%d", ip, ntohs(session->dst.port));
    json_object_set_new(object, "target", json_string(endpoint));

    json_object_set(object, "scanned", json_object_get(offender->portscan, "targets"));

    core_dispatch(&aggregate->user, 0, (uintptr_t) object);
    json_decref(object);
  }
}

static core_status event(core_event *event)
{
  aggregate *aggregate = event->state;

  if (event->type == portscan_type)
    portscan_event(aggregate, (json_t *) event->data);
  else if (event->type == session_type)
    session_event(aggregate, (session *) event->data);
  return CORE_OK;
}

static void *new (core_callback *callback, void *state, json_t *spec)
{
  aggregate *aggregate;

  (void) spec;
  aggregate = malloc(sizeof *aggregate);
  *aggregate = (struct aggregate) {.user = {.callback = callback, .state = state}};
  mapi_construct(&aggregate->offenders);
  return aggregate;
}

flow_table module_table = {.new = new, .event = event};
