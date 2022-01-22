#include <reactor.h>

#include "symbols.h"

void symbols_construct(symbols *symbols)
{
  *symbols = (struct symbols) {0};
  maps_construct(&symbols->map);
}

void symbols_register(symbols *symbols, const char *name)
{
  if (symbols_lookup(symbols, name))
    return;

  symbols->count++;
  maps_insert(&symbols->map, (char *) name, symbols->count, NULL);
}

int symbols_lookup(symbols *symbols, const char *name)
{
  return name ? maps_at(&symbols->map, (char *) name) : 0;
}

void symbols_destruct(symbols *symbols)
{
  maps_destruct(&symbols->map, NULL);
}
