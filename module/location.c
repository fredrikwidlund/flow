#include <dynamic.h>
#include <reactor.h>
#include <jansson.h>
#include <GeoIP.h>
#include <GeoIPCity.h>

#include "flow.h"

typedef struct location location;

struct location
{
  core_handler user;
  GeoIP *country;
  GeoIP *city;
  GeoIP *asn;
};

static core_status event(core_event *event)
{
  location *location = event->state;
  json_t *array, *object = (json_t *) event->data;
  uint32_t addr;
  GeoIPRecord *gir;

  if ((strcmp(json_string_value(json_object_get(object, "network")), "194.15.212.0") == 0) ||
      (strcmp(json_string_value(json_object_get(object, "network")), "37.203.216.0") == 0))
    return CORE_OK;

  inet_pton(AF_INET, json_string_value(json_object_get(object, "address")), &addr);
  json_object_set_new(object, "asn", json_string(GeoIP_name_by_ipnum(location->asn, ntohl(addr))));
  json_object_set_new(object, "country",
                      json_string(GeoIP_country_name[GeoIP_id_by_ipnum(location->country, ntohl(addr))]));
  gir = GeoIP_record_by_ipnum(location->city, ntohl(addr));
  if (gir)
  {
    json_object_set_new(object, "city", json_string(gir->city));

    array = json_array();
    json_array_append_new(array, json_real(gir->latitude));
    json_array_append_new(array, json_real(gir->longitude));
    json_object_set_new(object, "location", array);
    GeoIPRecord_delete(gir);
  }

  json_dumpf(object, stdout, JSON_COMPACT);
  (void) fprintf(stdout, "\n");
  fflush(stdout);
  return CORE_OK;
}

static void *new (core_callback *callback, void *state, json_t *spec)
{
  location *location;
  int i;

  (void) spec;
  location = malloc(sizeof *location);
  *location = (struct location) {.user = {.callback = callback, .state = state}};

  for (i = 0; i < NUM_DB_TYPES; i++)
    if (GeoIP_db_avail(i))
      switch (i)
      {
      case GEOIP_COUNTRY_EDITION:
        location->country = GeoIP_open_type(i, GEOIP_MEMORY_CACHE | GEOIP_CHECK_CACHE);
        GeoIP_set_charset(location->country, GEOIP_CHARSET_UTF8);
        break;
      case GEOIP_CITY_EDITION_REV1:
        location->city = GeoIP_open_type(i, GEOIP_MEMORY_CACHE | GEOIP_CHECK_CACHE);
        GeoIP_set_charset(location->city, GEOIP_CHARSET_UTF8);
        break;
      case GEOIP_ASNUM_EDITION:
        location->asn = GeoIP_open_type(i, GEOIP_MEMORY_CACHE | GEOIP_CHECK_CACHE);
        GeoIP_set_charset(location->asn, GEOIP_CHARSET_UTF8);
        break;
      }

  return location;
}

flow_table module_table = {.new = new, .event = event};
