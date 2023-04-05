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
    default:
      printf("?\n");
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
