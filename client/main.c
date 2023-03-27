#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <linux/if.h>
#include <linux/if_packet.h>
#include <linux/if_tun.h>

#include "akarivpn.h"
#include "netif.h"

extern int terminated;

struct client_opt {
  char *ifname;
  char *ipport;
};

struct vpn_client {
  int tap_fd;
  int server_fd;
};

static int
tap_init(struct vpn_client *cli, const char *tapname) {
  int fd;
  struct ifreq ifr = {0};

  if((fd = open("/dev/net/tun", O_RDWR)) < 0) {
    perror("open /dev/net/tun failed");
    return -1;
  }

  ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
  strncpy(ifr.ifr_name, tapname, sizeof(ifr.ifr_name) - 1);
  if(ioctl(fd, TUNSETIFF, &ifr) < 0) {
    perror("TUNSETIFF");
    return -1;
  }

  cli->tap_fd = fd;

  return 0;
}

static int
tcp_tunnel_init(struct vpn_client *cli, char *ip, uint16_t port) {
  int sock;
  struct sockaddr_in sin = {0};

  if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    return -1;
  }

  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);
  sin.sin_addr.s_addr = inet_addr(ip);

  if(connect(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
    perror("connect");
    return -1;
  }

  cli->server_fd = sock;
  printf("tcp connection done\n");

  return 0;
}

static int
clientloop(struct vpn_client *cli) {
  ;
  
  while(!terminated) {
    ;
  }

  return 0;
}

static int
do_vpn_client(struct client_opt *opt) {
  struct netif *netif = netif_init(opt->ifname, 1);
  struct vpn_client client;
  uint8_t *m;

  if(!netif)
    return -1;

  m = netif->hwaddr;
  printf("found network interface: %s hw %02x:%02x:%02x:%02x:%02x:%02x\n",
         netif->ifname, m[0], m[1], m[2], m[3], m[4], m[5]);

  if(tap_init(&client, "tap114514") < 0)
    return -1;
  if(tcp_tunnel_init(&client, "127.0.0.1", 1145) < 0)
    return -1;

  return clientloop(&client);
}

int
main(int argc, char **argv) {
  struct client_opt opt;

  if(argc < 2)
    return -1;

  opt.ipport = argv[1];
  opt.ifname = "wlp59s0";

  signal_init();

  return do_vpn_client(&opt);
}
