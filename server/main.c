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

struct client {
  int fd;
  struct in_addr addr;
  uint16_t port;
};

struct vpn_server {
  struct pollfd fds[2];
  int socket;
  struct client client;
};

static int
tcp_tunnel_prepare(struct vpn_server *serv) {
  int sock;
  int yes = 1;
  struct sockaddr_in sin;

  if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    return -1;
  }

  sin.sin_family = AF_INET;
  sin.sin_port = htons(1145);
  sin.sin_addr.s_addr = inet_addr("127.0.0.1");

  if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
    perror("setsockopt");
    return -1;
  }
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
  struct sockaddr_in client_sin;
  socklen_t sin_len = sizeof(client_sin);

  if((client_fd = accept(serv->socket, (struct sockaddr *)&client_sin, &sin_len)) < 0) {
    perror("accept");
    return -1;
  }
  if(sin_len <= 0) {
    perror("wtf");
    return -1;
  }
  
  serv->client.fd = client_fd;
  memcpy(&serv->client.addr, &client_sin.sin_addr, sizeof(serv->client.addr));
  serv->client.port = ntohs(client_sin.sin_port);

  return 0;
}

static void
client_disconnect(struct client *cli) {
  close(cli->fd);
}

static void
vpn_server_close(struct vpn_server *serv) {
  close(serv->socket);
}

static int
serverloop(struct vpn_server *serv) {
  int nready;

  while(!terminated) {
    const char *ip;

    printf("waiting connection...\n");
    if(tcp_accept(serv) < 0)
      return -1;

    ip = inet_ntoa(serv->client.addr);
    printf("connected: %d %s %d\n", serv->client.fd, ip, serv->client.port);

    client_disconnect(&serv->client);
  }

  vpn_server_close(serv);

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
  return do_vpn_server();
}
