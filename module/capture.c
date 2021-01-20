#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <err.h>
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

static void *offset(void *p, size_t offset)
{
  return ((uint8_t *) p) + offset;
}

static core_status event(core_event *event)
{
  capture *capture = event->state;
  struct tpacket_block_desc *bh;
  struct tpacket3_hdr *tp;
  capture_frame f;
  core_status e;

  if (event->data == EPOLLIN)
    while (1)
    {
      bh = offset(capture->data, capture->block * 128 * 4096);
      if (!bh->hdr.bh1.block_status & TP_STATUS_USER)
        break;
      tp = offset(bh, bh->hdr.bh1.offset_to_first_pkt);
      while (tp->tp_len)
      {
        f.sll = offset(tp, TPACKET_ALIGN(sizeof(struct tpacket3_hdr)));
        f.data = segment_data(offset(tp, tp->tp_mac), tp->tp_len);
        e = core_dispatch(&capture->user, CAPTURE_FRAME, (uintptr_t) &f);
        if (e)
          return e;
        if (tp->tp_next_offset == 0)
          break;
        tp = offset(tp, tp->tp_next_offset);
      }
      bh->hdr.bh1.block_status = TP_STATUS_KERNEL;
      capture->block++;
      capture->block %= 4;
    }
  return CORE_OK;
}

static void *new (core_callback *callback, void *state, json_t *spec)
{
  capture *capture;

  capture = malloc(sizeof *capture);
  *capture = (struct capture) {.user = {.callback = callback, .state = state}, .spec = spec, .fd = -1};
  return capture;
}

static void release(void *data)
{
  capture *capture = data;

  if (capture->fd >= 0)
  {
    core_delete(NULL, capture->fd);
    (void) close(capture->fd);
  }
  free(capture);
}

static void start(void *data)
{
  capture *capture = data;
  const char *name;
  int e;

  name = json_string_value(json_object_get(capture->spec, "interface"));
  assert(name);
  capture->interface = if_nametoindex(name);

  struct tpacket_req3 tp = {
      .tp_block_size = 128 * 4096, .tp_block_nr = 4, .tp_frame_size = 4096, .tp_frame_nr = 4 * 128};
  struct sockaddr_ll sll = {
      .sll_family = PF_PACKET, .sll_protocol = htons(ETH_P_ALL), .sll_ifindex = capture->interface};
  capture->fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  assert(capture->fd >= 0);
  e = setsockopt(capture->fd, SOL_PACKET, PACKET_VERSION, (int[]) {TPACKET_V3}, sizeof(int));
  assert(e != -1);
  e = setsockopt(capture->fd, SOL_PACKET, PACKET_RX_RING, &tp, sizeof tp);
  assert(e != -1);
  capture->data =
      mmap(NULL, 4096 * 128 * 4, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED | MAP_POPULATE, capture->fd, 0);
  assert(capture->data);
  memset(capture->data, 0, 4096 * 128 * 4);
  e = bind(capture->fd, (struct sockaddr *) &sll, sizeof sll);
  assert(e != -1);

  struct ifreq ifr = {0};
  if_indextoname(capture->interface, ifr.ifr_name);
  e = ioctl(capture->fd, SIOCGIFFLAGS, &ifr);
  if (e == -1)
    err(1, "ioctl");
  ifr.ifr_flags |= IFF_PROMISC;
  e = ioctl(capture->fd, SIOCSIFFLAGS, &ifr);
  if (e == -1)
    err(1, "ioctl");

  core_add(NULL, event, capture, capture->fd, EPOLLIN | EPOLLET);
}

flow_table module_table = {.new = new, .release = release, .start = start};
