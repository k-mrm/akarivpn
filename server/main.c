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
  struct pollfd fds[16];
  int nfds;
  int socket;
  int timeout;
  struct tuntap *tun;
  struct client *clients[14];
  int nclient;
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

static struct client *
tcp_accept(struct vpn_server *serv) {
  int client_fd;
  struct client *cli = NULL;
  struct sockaddr_in client_sin;
  socklen_t sin_len = sizeof(client_sin);

  cli = malloc(sizeof(*cli));
  if(!cli)
    return NULL;

  if((client_fd = accept(serv->socket, (struct sockaddr *)&client_sin, &sin_len)) < 0) {
    perror("accept");
    goto err;
  }
  if(sin_len <= 0) {
    perror("wtf");
    goto err;
  }
  
  cli->fd = client_fd;
  memcpy(&cli->addr, &client_sin.sin_addr, sizeof(cli->addr));
  cli->port = ntohs(client_sin.sin_port);

  return cli;

err:
  free(cli);
  return NULL;
}

static void
client_pollfd(struct vpn_server *serv, struct client *cli, int ev) {
  serv->fds[serv->nfds] = (struct pollfd){
    .fd = cli->fd,
    .events = ev,
  };

  serv->nfds++;
}

static int
connect_from_client(struct vpn_server *serv) {
  struct client *client;

  if((client = tcp_accept(serv)) == NULL)
    return -1;

  client_pollfd(serv, client, POLLIN);

  serv->clients[serv->nclient] = client;
  serv->nclient++;

  printf("new client: %s\n", inet_ntoa(client->addr));

  return 0;
}

static void
client_disconnect(struct client *cli) {
  printf("client disconnected\n");
  close(cli->fd);
}

static void
vpn_server_close(struct vpn_server *serv) {
  close(serv->socket);
}

static int
pollfd_prepare(struct vpn_server *serv) {
  serv->fds[0] = (struct pollfd){
    .fd = serv->socket,
    .events = POLLIN,
  };
  serv->fds[1] = (struct pollfd){
    .fd = serv->tun->fd,
    .events = POLLIN,
  };
  serv->nfds = 2;

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
        if(connect_from_client(serv) < 0)
          return -1;
      } else if(i == 1) {   // tap
        if((size = tun_read(serv->tun, buf, sizeof(buf))) <= 0) {
          perror("tunread");
          return -1;
        }

        if((wsize = write(serv->clients[0]->fd, buf, size)) <= 0) {
          perror("write");
          return -1;
        }
      } else {              // client
        int cli_idx = i - 2;
        struct client *cli = serv->clients[cli_idx];
        printf("from client: %s\n", inet_ntoa(cli->addr));

        if((size = read(cli->fd, buf, sizeof(buf))) <= 0) {
          client_disconnect(cli);
          return 0;
        }

        l2packet_dump(buf, size);

        if((wsize = tun_write(serv->tun, buf, size)) <= 0) {
          perror("tunwrite");
          return -1;
        }
      }
    }
  }

  return 0;
}

static int
serverloop(struct vpn_server *serv) {
  if(pollfd_prepare(serv) < 0)
    return -1;

  while(!terminated) {
    printf("waiting connection...\n");
    if(connect_from_client(serv) < 0)
      return -1;

    while(server(serv) == 0)
      ;
  }

  vpn_server_close(serv);

  return 0;
}

static int
do_vpn_server() {
  struct vpn_server serv = {0};

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
