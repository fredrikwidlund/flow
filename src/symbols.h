#ifndef SYMBOLS_H_INCLUDED
#define SYMBOLS_H_INCLUDED

#include <reactor.h>

typedef struct symbols symbols;

struct symbols
{
  maps map;
  int  count;
};

void symbols_construct(symbols *);
void symbols_register(symbols *, const char *);
int  symbols_lookup(symbols *, const char *);
void symbols_destruct(symbols *);

#endif /* SYMBOLS_H_INCLUDED */
