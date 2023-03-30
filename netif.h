#ifndef AKARIVPN_NETIF_H
#define AKARIVPN_NETIF_H

struct netif {
  int socket;
  struct ifreq *ifr;
  const char *ifname;
  unsigned char hwaddr[6];
};

void netif_err(struct netif *netif, const char *msg);
int netif_set_ifflags(struct netif *netif, int flags);
void hwaddrdump(unsigned char *hwaddr);
struct netif *netif_init(const char *ifname, int promisc);
int disable_ip_forward(void);
void packetdump(struct netif *netif, unsigned char *buf, size_t size);
void l3packet_dump(unsigned char *packet, size_t size, unsigned short type);
void l2packet_dump(unsigned char *packet, size_t size);

#endif
