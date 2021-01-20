#define _GNU_SOURCE

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
#include <netinet/udp.h>
#include <netinet/ether.h>
#include <netinet/if_ether.h>
#include <linux/if_packet.h>

#include <dynamic.h>
#include <reactor.h>
#include <jansson.h>

#include "flow.h"
#include "capture.h"
#include "protocol.h"

/*
char *ether_ntoa_r(const struct ether_addr *addr, char *buf)
{
  sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x", addr->ether_addr_octet[0], addr->ether_addr_octet[1],
          addr->ether_addr_octet[2], addr->ether_addr_octet[3], addr->ether_addr_octet[4], addr->ether_addr_octet[5]);
  return buf;
}
*/

static void debug_sll(struct sockaddr_ll *sll)
{
  char interface[IF_NAMESIZE];
  char *type[] = {"<", "*", "=", "-", ">"};

  if (sll->sll_pkttype < sizeof type / sizeof *type)
    (void) fprintf(stderr, "[%s %s]", type[sll->sll_pkttype], if_indextoname(sll->sll_ifindex, interface));
  else
    (void) fprintf(stderr, "[%d %s]", sll->sll_pkttype, if_indextoname(sll->sll_ifindex, interface));
}

static void debug_ether(struct ether_header *ether)
{
  char sha[32], tha[32];

  (void) fprintf(stderr, "[%s %s]", ether_ntoa_r((struct ether_addr *) ether->ether_shost, sha),
                 ether_ntoa_r((struct ether_addr *) ether->ether_dhost, tha));
}

static void debug_arp(struct arphdr *arp)
{
  struct ether_arp *ea = (struct ether_arp *) arp;
  char *type[] = {"request", "reply"}, spa[INET_ADDRSTRLEN], tpa[INET_ADDRSTRLEN], sha[32], tha[32];
  short op = ntohs(arp->ar_op);

  if (ntohs(arp->ar_op) > 1 || ntohs(arp->ar_hrd) != ARPHRD_ETHER || arp->ar_hln != ETH_ALEN ||
      ntohs(arp->ar_pro) != ETHERTYPE_IP || arp->ar_pln != 4)
    (void) fprintf(stderr, "[arp unknown]");
  else
  {
    (void) fprintf(stderr, "[%s %s,%s->%s,%s]", type[op], inet_ntop(AF_INET, ea->arp_spa, spa, sizeof spa),
                   ether_ntoa_r((struct ether_addr *) ea->arp_sha, sha),
                   inet_ntop(AF_INET, ea->arp_tpa, tpa, sizeof tpa),
                   ether_ntoa_r((struct ether_addr *) ea->arp_tha, tha));
  }
}

static void debug_ip(struct iphdr *ip)
{
  char src[INET_ADDRSTRLEN], dst[INET_ADDRSTRLEN], storage[32], *proto;
  char *map[256] = {
      [IPPROTO_ICMP] = "ICMP", [IPPROTO_TCP] = "TCP", [IPPROTO_UDP] = "UDP", [IPPROTO_ESP] = "ESP", [112] = "VRRP"};

  proto = map[ip->protocol];
  if (!proto)
  {
    snprintf(storage, sizeof storage, "%02x", ip->protocol);
    proto = storage;
  }
  (void) fprintf(stderr, "[%s %s %s]", inet_ntop(AF_INET, &ip->saddr, src, sizeof src),
                 inet_ntop(AF_INET, &ip->daddr, dst, sizeof dst), proto);
}

void debug_tcp(struct tcphdr *th)
{
  char flags[] = "FSRPAU", tmp[8] = {0};
  size_t i, j;

  for (i = 0, j = 0; i < strlen(flags); i++)
    if (th->th_flags & (1 << i))
    {
      tmp[j] = flags[i];
      j++;
    }
  (void) fprintf(stderr, "[%d %d %s]", ntohs(th->th_sport), ntohs(th->th_dport), tmp);
}

static void debug_udp(struct udphdr *uh)
{
  (void) fprintf(stderr, "[%d %d]", ntohs(uh->uh_sport), ntohs(uh->uh_dport));
}

static void debug(protocol_headers *headers)
{
  size_t i;
  protocol_header *h;

  for (i = 0, h = &headers->header[i]; i < headers->count; i++, h++)
    switch (h->type)
    {
    case PROTOCOL_SLL:
      debug_sll(h->sll);
      break;
    case PROTOCOL_ETHER:
      debug_ether(h->ether);
      break;
    case PROTOCOL_ARP:
      debug_arp(h->arp);
      break;
    case PROTOCOL_IP:
      debug_ip(h->ip);
      break;
    case PROTOCOL_TCP:
      debug_tcp(h->tcp);
      break;
    case PROTOCOL_UDP:
      debug_udp(h->udp);
      break;
    default:
      (void) fprintf(stderr, "[DATA %lu]", h->size);
      break;
    }
  (void) fprintf(stderr, "\n");
}

static void add(protocol_headers *headers, int type, size_t size, void *data)
{
  if (headers->count < PROTOCOL_HEADERS_MAX)
  {
    headers->header[headers->count] = (protocol_header) {.type = type, .size = size, .data = data};
    headers->count++;
  }
}

static segment add_data(protocol_headers *headers, segment data)
{
  add(headers, PROTOCOL_DATA, data.size, data.base);
  return segment_empty();
}

static segment add_udp(protocol_headers *headers, segment data)
{
  struct udphdr *udp = data.base;
  size_t header_len = sizeof *udp;

  if (data.size < header_len)
    return data;
  data = segment_offset(data, header_len);
  add(headers, PROTOCOL_UDP, header_len, udp);
  return data;
}

static segment add_tcp(protocol_headers *headers, segment data)
{
  struct tcphdr *tcp = data.base;
  size_t header_len;

  if (data.size < sizeof *tcp)
    return data;
  header_len = tcp->th_off << 2;
  if (data.size < header_len)
    return data;
  data = segment_offset(data, header_len);
  add(headers, PROTOCOL_TCP, header_len, tcp);
  return data;
}

static segment add_ip(protocol_headers *headers, segment data)
{
  struct iphdr *ip = data.base;
  size_t header_len, total_len;

  if (data.size < sizeof *ip)
    return data;
  header_len = ip->ihl << 2;
  total_len = ntohs(ip->tot_len);
  if (data.size < header_len || data.size < total_len || total_len < header_len)
    return data;
  data.size = total_len;
  data = segment_offset(data, header_len);

  add(headers, PROTOCOL_IP, header_len, ip);
  switch (ip->protocol)
  {
  case IPPROTO_TCP:
    return add_tcp(headers, data);
  case IPPROTO_UDP:
    return add_udp(headers, data);
  default:
    return data;
  }
}

static segment add_arp(protocol_headers *headers, segment data)
{
  struct arphdr *ar = data.base;
  size_t header_len;

  if (data.size < sizeof *ar)
    return data;
  header_len = sizeof *ar + (2 * ar->ar_hln) + (2 * ar->ar_pln);
  if (data.size < header_len)
    return data;
  data.size = header_len;
  data = segment_offset(data, header_len);

  add(headers, PROTOCOL_ARP, header_len, ar);
  return data;
}

static segment add_ether(protocol_headers *headers, segment data)
{
  struct ether_header *ether = data.base;

  if (data.size < sizeof *ether)
    return data;
  data = segment_offset(data, sizeof *ether);

  add(headers, PROTOCOL_ETHER, sizeof *ether, ether);
  switch (ntohs(ether->ether_type))
  {
  case ETHERTYPE_IP:
    return add_ip(headers, data);
  case ETHERTYPE_ARP:
    return add_arp(headers, data);
  default:
    return data;
  }
}

static void add_frame(protocol_headers *headers, capture_frame *frame)
{
  segment remaining = frame->data;

  add(headers, PROTOCOL_SLL, sizeof *frame->sll, frame->sll);
  switch (frame->sll->sll_hatype)
  {
  case ARPHRD_LOOPBACK:
  case ARPHRD_ETHER:
    remaining = add_ether(headers, remaining);
    break;
  }

  add_data(headers, remaining);
}

static core_status event(core_event *event)
{
  protocol *protocol = event->state;
  capture_frame *frame = (capture_frame *) event->data;
  protocol_headers headers = {0};

  add_frame(&headers, frame);
  if (protocol->debug)
    debug(&headers);
  return core_dispatch(&protocol->user, PROTOCOL_FRAME, (uintptr_t) &headers);
}

static void *new (core_callback *callback, void *state, json_t *spec)
{
  protocol *protocol;

  protocol = malloc(sizeof *protocol);
  *protocol = (struct protocol) {.user = {.callback = callback, .state = state},
                                 .debug = json_is_true(json_object_get(spec, "debug"))};
  return protocol;
}

flow_table module_table = {.new = new, .event = event};
