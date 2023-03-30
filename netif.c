#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/if_ether.h>
#include <linux/if_packet.h>

#include "akarivpn.h"
#include "netif.h"

void
netif_err(struct netif *netif, const char *msg) {
  printf("%s(%d): ", netif->ifname, netif->socket);
  perror(msg);
}

int
netif_set_ifflags(struct netif *netif, int flags) {
  if(ioctl(netif->socket, SIOCGIFFLAGS, netif->ifr) < 0) {
    netif_err(netif, "gifflags");
    return -1;
  }

  netif->ifr->ifr_flags |= flags;

  if(ioctl(netif->socket, SIOCSIFFLAGS, netif->ifr) < 0) {
    netif_err(netif, "sifflags");
    return -1;
  }

  return 0;
}

static int
netif_init_macaddr(struct netif *netif) {
  if(ioctl(netif->socket, SIOCGIFHWADDR, netif->ifr) < 0)
    return -1;
  memcpy(netif->hwaddr, netif->ifr->ifr_hwaddr.sa_data, 6);

  return 0;
}

void
hwaddrdump(unsigned char *hwaddr) {
  printf("macaddr: %2x:%2x:%2x:%2x:%2x:%2x\n",
         hwaddr[0], hwaddr[1], hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]);
}

struct netif *
netif_init(const char *ifname, int promisc) {
  int soc;
  struct sockaddr_ll sa;
  struct netif *netif = malloc(sizeof(*netif));
  struct ifreq *ifr = malloc(sizeof(*ifr));

  if(!netif || !ifr)
    return NULL;

  netif->ifname = ifname;
  netif->ifr = ifr;

  if((soc = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
    netif_err(netif, "socket");
    goto err;
  }
  netif->socket = soc;

  strncpy(ifr->ifr_name, ifname, sizeof(ifr->ifr_name) - 1);
  if(ioctl(soc, SIOCGIFINDEX, ifr) < 0) {
    netif_err(netif, "ioctl");
    goto err;
  }

  sa.sll_family = PF_PACKET;
  sa.sll_protocol = htons(ETH_P_ALL);
  sa.sll_ifindex = ifr->ifr_ifindex;

  if(bind(soc, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
    netif_err(netif, "bind");
    goto err;
  }

  if(promisc) {
    if(netif_set_ifflags(netif, IFF_PROMISC) < 0)
      goto err;
  }

  netif_init_macaddr(netif);

  return netif;

err:
  free(netif);
  free(ifr);
  close(soc);

  return NULL;
}

void
netif_free(struct netif *netif) {
  close(netif->socket);
  free(netif);
}

int
disable_ip_forward() {
  FILE *fp;

  if(!(fp = fopen("/proc/sys/net/ipv4/ip_forward", "w")))
    return -1;
  fputs("0", fp);
  fclose(fp);

  return 0;
}

static void
ipv4addrfmt(uint32_t ipaddr, char *str) {
  uint8_t *ip = (uint8_t *)&ipaddr;

  snprintf(str, 32, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

static void
ipv6addrfmt(struct in6_addr *ipaddr, char *str) {
  uint16_t *addr = (uint16_t *)ipaddr->s6_addr;

  snprintf(str, 64, "%x:%x:%x:%x:%x:%x:%x:%x",
           ntohs(addr[0]), ntohs(addr[1]), ntohs(addr[2]), ntohs(addr[3]),
           ntohs(addr[4]), ntohs(addr[5]), ntohs(addr[6]), ntohs(addr[7]));
}

static const char *
ip_protocol_fmt(uint8_t proto) {
  switch(proto) {
    case IPPROTO_TCP:
      return "tcp";
    case IPPROTO_UDP:
      return "udp";
    default:
      return "???";
  }
}

static void
ipv4_packet_dump(unsigned char *buf) {
  struct iphdr *iphdr = (struct iphdr *)buf;
  char dst[32] = {0};
  char src[32] = {0};

  ipv4addrfmt(iphdr->daddr, dst);
  ipv4addrfmt(iphdr->saddr, src);

  printf("ipv4 %s ---> %s %s\n", src, dst, ip_protocol_fmt(iphdr->protocol));
}

static void
ipv6_packet_dump(unsigned char *buf) {
  struct ip6_hdr *ip6 = (struct ip6_hdr *)buf;
  struct in6_addr *dst = &ip6->ip6_dst;
  struct in6_addr *src = &ip6->ip6_src;
  char d_str[64];
  char s_str[64];

  ipv6addrfmt(dst, d_str);
  ipv6addrfmt(src, s_str);

  printf("ipv6 %s ---> %s %s\n", s_str, d_str, ip_protocol_fmt(ip6->ip6_nxt));
}

void
l3packet_dump(unsigned char *packet, size_t size, unsigned short type) {
  switch(type) {
    case 0x0800:    // ipv4
      ipv4_packet_dump(packet);
      break;
    case 0x86dd:    // ipv6
      ipv6_packet_dump(packet);
      break;
  };
}

void
l2packet_dump(unsigned char *packet, size_t size) {
  struct ether_header *eth = (struct ether_header *)packet;
  unsigned short type = ntohs(eth->ether_type);

  packet += sizeof(struct ether_header);
  size -= sizeof(struct ether_header);

  printf("ether type %#x size %ld ", type, size);
  l3packet_dump(packet, size, type);
}

void
packetdump(struct netif *netif, unsigned char *buf, size_t size) {
  printf("-------- packet to %s\n", netif->ifname);
  for(int i = 0; i < size; i++) {
    printf("%02x ", buf[i]);
    if((i + 1) % 16 == 0)
      printf("\n");
  }
}
