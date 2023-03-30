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
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <linux/if_packet.h>

#include "akarivpn.h"
#include "netif.h"
#include "tuntap.h"

extern int terminated;

struct client_opt {
  char *ifname;
  char *ipport;
};

struct vpn_client {
  struct tunnel *tun;
  int server_fd;
  struct pollfd fds[2];
  int nfds;
};

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
clientcore(struct vpn_client *cli) {
  int nready = poll(cli->fds, cli->nfds, 2000);
  ssize_t size;
  char buf[2048];

  if(nready < 0)
    return -1;

  for(int i = 0; i < cli->nfds && nready; i++) {
    struct pollfd *fd = &cli->fds[i];
    if(fd->revents & POLLIN) {
      nready--;

      if(i == 0) {    // tun
        if((size = tun_read(cli->tun, buf, sizeof(buf))) <= 0) {
          perror("tunread");
          return -1;
        }
        for(int c = 0; c < 64; c++) {
          printf("%02x ", buf[c]);
        }
      } else {        // server
        printf("from server\n");
        return -1;
      }
    }
  }

  return 0;
}

static int
clientloop(struct vpn_client *cli) {
  cli->fds[0] = (struct pollfd){
    .fd = cli->tun->fd,
    .events = POLLIN,
  };
  cli->fds[1] = (struct pollfd){
    .fd = cli->server_fd,
    .events = POLLIN,
  };
  cli->nfds = 2;

  while(!terminated && clientcore(cli) == 0)
    ;

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

  if((client.tun = tun_alloc("tap114514")) == NULL) {
    printf("wtf\n");
    return -1;
  }

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
