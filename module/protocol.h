#ifndef PROTOCOL_H_INCLUDED
#define PROTOCOL_H_INCLUDED

#define PROTOCOL_HEADERS_MAX 8

enum
{
  PROTOCOL_ERROR = 0,
  PROTOCOL_FRAME
};

enum
{
  PROTOCOL_NONE = 0,
  PROTOCOL_SLL,
  PROTOCOL_ETHER,
  PROTOCOL_ARP,
  PROTOCOL_IP,
  PROTOCOL_TCP,
  PROTOCOL_UDP,
  PROTOCOL_DATA
};

typedef struct protocol_headers protocol_headers;
typedef struct protocol_header  protocol_header;
typedef struct protocol         protocol;

struct protocol_header
{
  int                    type;
  size_t                 size;
  union
  {
    void                *data;
    struct sockaddr_ll  *sll;
    struct ether_header *ether;
    struct arphdr       *arp;
    struct iphdr        *ip;
    struct tcphdr       *tcp;
    struct udphdr       *udp;
  };
};

struct protocol_headers
{
  size_t                 count;
  protocol_header        header[PROTOCOL_HEADERS_MAX];
};

struct protocol
{
  core_handler           user;
  int                    debug;
};

void protocol_debug_tcp(struct tcphdr *);

#endif /* PROTOCOL_H_INCLUDED */
