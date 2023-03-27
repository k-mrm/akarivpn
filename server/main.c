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

extern int terminated;

struct vpn_server {
  int socket;
  int client_fd;
};

static int
tcp_tunnel_prepare(struct vpn_server *serv) {
  int sock;
  struct sockaddr_in sin;

  if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    return -1;
  }

  sin.sin_family = AF_INET;
  sin.sin_port = htons(1145);
  sin.sin_addr.s_addr = inet_addr("127.0.0.1");

  if(bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
    perror("bind");
    return -1;
  }
  if(listen(sock, 5) < 0) {
    perror("listen");
    return -1;
  }

  serv->socket = sock;

  return 0;
}

static int
tcp_accept(struct vpn_server *serv) {
  int client_fd;

  if((client_fd = accept(serv->socket, NULL, NULL)) < 0) {
    perror("accept");
    return -1;
  }
  
  serv->client_fd = client_fd;
  return 0;
}

static int
serverloop(struct vpn_server *serv) {
  while(!terminated) {
    printf("waiting connection...\n");
    if(tcp_accept(serv) < 0)
      return -1;

    printf("connected: %d\n", serv->client_fd);
    return 0;
  }

  return 0;
}

static int
do_vpn_server() {
  struct vpn_server serv;

  if(tcp_tunnel_prepare(&serv) < 0)
    return -1;

  return serverloop(&serv);
}

int
main(int argc, char **argv) {
  signal_init();

  return do_vpn_server();
}
