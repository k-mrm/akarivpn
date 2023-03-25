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
#include <netinet/if_ether.h>
#include <linux/if_packet.h>

#include "akarivpn.h"
#include "netif.h"

static int
vpn_client(const char *intf) {
  struct netif *netif = netif_init(intf, 1);
  uint8_t *m;

  if(!netif) {
    akrvpn_err("null netif");
    return -1;
  }

  m = netif->hwaddr;
  printf("network interface: %s hw %02x:%02x:%02x:%02x:%02x:%02x\n",
         netif->ifname, m[0], m[1], m[2], m[3], m[4], m[5]);

  return 0;
}

int
main(int argc, char **argv) {
  if(argc < 2) {
    akrvpn_info("akr-client <if>");
    akrvpn_err("aborted");
    return -1;
  }

  return vpn_client(argv[1]);
}
