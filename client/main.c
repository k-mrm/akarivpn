#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <netdb.h>
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
#include <getopt.h>

#include "akarivpn.h"
#include "netif.h"
#include "tuntap.h"
#include "tunnel.h"

#define EV_C_NULL     0
#define EV_C_SERVER   1
#define EV_C_TUNTAP   2

extern int terminated;

struct vpn_client {
  struct tuntap *tt;
  struct events events;
  struct tunnel *tunnel;
};

static int
clientcore(struct vpn_client *cli) {
  ssize_t size;
  unsigned char buf[2048];
  int nready;
  struct revent revs[16];

  if((nready = events_poll_pollin(&cli->events, 2000, revs)) < 0) 
    return -1;

  for(int i = 0; i < nready; i++) {
    void *priv = revs[i].priv;

    switch((size_t)priv) {
      case EV_C_SERVER:
        if((size = tcp_tunnel_read(cli->tunnel, buf, sizeof(buf))) <= 0) {
          tunnel_disconnected(cli->tunnel);
          return 0;
        }
        printf("ssss: ");
        l2packet_dump(buf, size);
        if(tun_write(cli->tt, buf, size) <= 0) {
          return -1;
        }
        break;
      case EV_C_TUNTAP:
        if((size = tun_read(cli->tt, buf, sizeof(buf))) <= 0) {
          return -1;
        }
        printf("tttt: ");
        l2packet_dump(buf, size);
        if(tcp_tunnel_write(cli->tunnel, buf, size) <= 0) {
          return -1;
        }
        break;
      default:
        puts("?");
        return -1;
    }
  }

  return 0;
}

static int
do_vpn_client(char *host, char *port, char *myip) {
  struct vpn_client client = {0};
  struct tunnel *tunnel;

  if((client.tt = tun_alloc("tap114514", IFF_TAP)) == NULL) {
    printf("wtf\n");
    return -1;
  }
  tun_setup(client.tt, myip, "255.255.255.0");

  tunnel = connect_tunnel(host, port, PROTO_TCP);
  if(!tunnel)
    return -1;

  add_event(&client.events, client.tt->fd, (void *)EV_C_TUNTAP);
  add_event(&client.events, tunnel->fd, (void *)EV_C_SERVER);

  client.tunnel = tunnel;
  printf("client: setup done\n");

  while(!terminated && clientcore(&client) == 0)
    ;

  return 0;
}

int
main(int argc, char **argv) {
  int opt;
  char *serv_ip = NULL, *myip = NULL, *port = NULL;

  while((opt = getopt(argc, argv, "i:s:p:")) != -1) {
    switch(opt) {
      case 'i':
        myip = optarg;
        break;
      case 's':
        serv_ip = optarg;
        break;
      case 'p':
        port = optarg;
        break;
      case '?':
        printf("parse error\n");
        return -1;
      default:
        break;
    }
  }

  if(!serv_ip) {
    printf("server ip?\n");
    return -1;
  }
  if(!myip) {
    puts("?");
    return -1;
  }
  if(!port)
    port = "1145";    // default port

  signal_init();

  return do_vpn_client(serv_ip, port, myip);
}
