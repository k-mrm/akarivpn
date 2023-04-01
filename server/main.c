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
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <linux/if_packet.h>

#include "akarivpn.h"
#include "netif.h"
#include "tuntap.h"

extern int terminated;

struct client {
  int fd;
  struct in_addr addr;
  uint16_t port;
};

struct vpn_server {
  struct pollfd fds[3];
  int nfds;
  int socket;
  int timeout;
  struct tuntap *tun;
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
  sin.sin_addr.s_addr = htonl(INADDR_ANY);

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
pollfd_init(struct vpn_server *serv) {
  serv->fds[0] = (struct pollfd){
    .fd = serv->socket,
    .events = POLLIN,
  };
  serv->fds[1] = (struct pollfd){
    .fd = serv->client.fd,
    .events = POLLIN,
  };
  serv->fds[2] = (struct pollfd){
    .fd = serv->tun->fd,
    .events = POLLIN,
  };
  serv->nfds = 3;

  return 0;
}

static int
server(struct vpn_server *serv) {
  int nready = poll(serv->fds, serv->nfds, 2000);
  ssize_t size, wsize;
  unsigned char buf[2048];

  if(nready < 0)
    return -1;

  for(int i = 0; i < serv->nfds && nready; i++) {
    struct pollfd *fd = &serv->fds[i];
    if(fd->revents & POLLIN) {
      nready--;

      if(i == 0) {          // listener port
        printf("connected from client\n");
      } else if(i == 1) {   // client
        if((size = read(fd->fd, buf, sizeof(buf))) <= 0) {
          printf("client disconnected\n");
          return -1;
        }

        l2packet_dump(buf, size);

        if((wsize = tun_write(serv->tun, buf, size)) <= 0) {
          perror("tunwrite");
          return -1;
        }
      } else {              // tap
        if((size = tun_read(serv->tun, buf, sizeof(buf))) <= 0) {
          perror("tunread");
          return -1;
        }

        if((wsize = write(serv->client.fd, buf, size)) <= 0) {
          perror("write");
          return -1;
        }
      }
    }
  }

  return 0;
}

static int
serverloop(struct vpn_server *serv) {
  while(!terminated) {
    const char *ip;

    printf("waiting connection...\n");
    if(tcp_accept(serv) < 0)
      return -1;
    ip = inet_ntoa(serv->client.addr);
    printf("connected: %d %s %d\n", serv->client.fd, ip, serv->client.port);

    if(pollfd_init(serv) < 0)
      return -1;

    while(server(serv) == 0)
      ;

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
  if((serv.tun = tun_alloc("tap1919", IFF_TAP)) == NULL) {
    printf("wtf\n");
    return -1;
  }
  tun_setup(serv.tun, "192.168.1.1", "255.255.255.0");

  return serverloop(&serv);
}

int
main(int argc, char **argv) {
  return do_vpn_server();
}
