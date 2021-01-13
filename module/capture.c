#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>

#include <dynamic.h>
#include <reactor.h>
#include <jansson.h>

#include "flow.h"
#include "capture.h"

typedef struct capture capture;
struct capture
{
  flow_node *node;
  int interface;
  int fd;
  int block;
  void *data;
};

static void *offset(void *p, size_t offset)
{
  return ((uint8_t *) p) + offset;
}

static core_status packets(core_event *event)
{
  capture *c = event->state;
  struct tpacket_block_desc *bh;
  struct tpacket3_hdr *tp;
  capture_frame f;

  if (event->data == EPOLLIN)
    while (1)
    {
      bh = offset(c->data, c->block * 128 * 4096);
      if (!bh->hdr.bh1.block_status & TP_STATUS_USER)
        break;
      tp = offset(bh, bh->hdr.bh1.offset_to_first_pkt);
      while (tp->tp_len)
      {
        f.sll = offset(tp, TPACKET_ALIGN(sizeof(struct tpacket3_hdr)));
        f.data = segment_data(offset(tp, tp->tp_mac), tp->tp_len);
        flow_send(c->node, &f);
        if (tp->tp_next_offset == 0)
          break;
        tp = offset(tp, tp->tp_next_offset);
      }
      bh->hdr.bh1.block_status = TP_STATUS_KERNEL;
      c->block++;
      c->block %= 4;
    }
  return CORE_OK;
}

static int setup(capture *c)
{
  struct ifreq ifr = {0};
  int e;

  e = setsockopt(c->fd, SOL_PACKET, PACKET_VERSION, (int[]) {TPACKET_V3}, sizeof(int));
  if (e == -1)
    return -1;
  e = setsockopt(c->fd, SOL_PACKET, PACKET_RX_RING, (struct tpacket_req3[]) {{128 * 4096, 4, 4096, 4 * 128, 0, 0, 0}},
                 sizeof(struct tpacket_req3));
  if (e == -1)
    return -1;
  c->data = mmap(NULL, 4096 * 128 * 4, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED | MAP_POPULATE, c->fd, 0);
  if (!c->data)
    return -1;
  memset(c->data, 0, 4096 * 128 * 4);
  e = bind(c->fd,
           (struct sockaddr *) (struct sockaddr_ll[]) {{PF_PACKET, htons(ETH_P_ALL), c->interface, 0, 0, 0, {0}}},
           sizeof(struct sockaddr_ll));
  if (e == -1)
    return -1;

  if_indextoname(c->interface, ifr.ifr_name);
  e = ioctl(c->fd, SIOCGIFFLAGS, &ifr);
  if (e == -1)
    return -1;
  ifr.ifr_flags |= IFF_PROMISC;
  e = ioctl(c->fd, SIOCSIFFLAGS, &ifr);
  if (e == -1)
    return -1;

  return 0;
}

static void *create(flow_node *node, json_t *spec)
{
  const char *name;
  capture *c;
  int i;

  name = json_string_value(json_object_get(spec, "interface"));
  if (!name)
    return NULL;
  i = if_nametoindex(name);
  if (!i)
    return NULL;

  c = calloc(1, sizeof *c);
  c->node = node;
  c->interface = i;
  return c;
}

static int start(void *data)
{
  capture *c = data;
  int e;

  c->fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if (c->fd == -1)
    return -1;
  e = setup(c);
  if (e == -1)
  {
    (void) close(c->fd);
    c->fd = -1;
    return -1;
  }

  core_add(NULL, packets, c, c->fd, EPOLLIN | EPOLLET);
  return 0;
}

static void destroy(void *data)
{
  free(data);
}

flow_module_handlers module_handlers = {.create = create, .start = start, .destroy = destroy};
