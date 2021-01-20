#ifndef PORTSCAN_H_INCLUDED
#define PORTSCAN_H_INCLUDED

enum
{
  PORTSCAN_ERROR = 0,
  PORTSCAN_ALERT
};

typedef struct portscan_node portscan_node;
typedef struct portscan      portscan;

struct portscan_node
{
  uint32_t      network;
  uint32_t      address;
  int           peer_limit_reached;
  size_t        peer_count;
  mapi          peers;
};

struct portscan
{
  core_handler  user;
  int           debug;
  mapi          nodes;
};

extern int portscan_type;

#endif /* PORTSCAN_H_INCLUDED */
