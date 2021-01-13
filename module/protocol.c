#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>

#include <dynamic.h>
#include <reactor.h>
#include <jansson.h>

#include "flow.h"
#include "capture.h"

/*
receive_capture_frame(void *state, capture_frame *frame)
{
}
*/

static void debug_ip(segment data)
{
  struct iphdr *ip = data.base;
  char src[16], dst[16];
  // size_t n;

  if (data.size < sizeof *ip)
    return;

  inet_ntop(AF_INET, &ip->saddr, src, sizeof src);
  inet_ntop(AF_INET, &ip->daddr, dst, sizeof dst);
  switch (ip->protocol)
  {
  case IPPROTO_TCP:
    (void) fprintf(stderr, "[tcp %s %s]", src, dst);
    break;
  case IPPROTO_UDP:
    (void) fprintf(stderr, "[udp %s %s]", src, dst);
    break;
  default:
    (void) fprintf(stderr, "[%02x %s %s]", ip->protocol, src, dst);
    break;
  }
}

static void debug_ether(segment data)
{
  struct ether_header *ether = data.base;
  size_t n = sizeof *ether;
  int type;

  if (data.size < n)
    return;
  type = ntohs(ether->ether_type);
  switch (type)
  {
  case ETHERTYPE_IP:
    (void) fprintf(stderr, "[ip]");
    debug_ip(segment_offset(data, n));
    break;
  default:
    (void) fprintf(stderr, "[0x%04x]", ntohs(ether->ether_type));
    break;
  }
}

static void debug(capture_frame *f)
{
  char name[IF_NAMESIZE];
  char *type[8] = {[PACKET_HOST] = "host", [PACKET_OUTGOING] = "out"};

  (void) fprintf(stderr, "[%s %s]", if_indextoname(f->sll->sll_ifindex, name), type[f->sll->sll_pkttype]);
  switch (f->sll->sll_hatype)
  {
  case ARPHRD_LOOPBACK:
  case ARPHRD_ETHER:
    debug_ether(f->data);
    break;
  }
  (void) fprintf(stderr, "\n");
}

static void receive(void *state, void *data)
{
  (void) state;
  debug(data);
}

flow_module_handlers module_handlers = {.receive = receive};
