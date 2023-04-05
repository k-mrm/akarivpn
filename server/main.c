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
#include "server/server.h"
#include "server/client.h"

extern int terminated;

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
servercore(struct vpn_server *serv) {
  int nready = events_poll(&serv->events, 2000);
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
          client_disconnect(serv, cli);
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
  while(!terminated) {
    printf("waiting connection...\n");
    if(tcp_tunnel_accept(serv->listen_fd) < 0)
      return -1;

    while(servercore(serv) == 0)
      ;
  }

  vpn_server_close(serv);

  return 0;
}

static int
do_vpn_server(void) {
  struct vpn_server serv = {0};
  int listen_fd;

  if((listen_fd = tunnel_listen(1145)) < 0)
    return -1;
  serv.listen_fd = listen_fd;

  if((serv.tun = tun_alloc("tap1919", IFF_TAP)) == NULL) {
    printf("wtf\n");
    return -1;
  }
  tun_setup(serv.tun, "192.168.1.1", "255.255.255.0");

  add_event(&serv->events, listen_fd);
  add_event(&serv->events, serv.tun->fd);

  return serverloop(&serv);
}

int
main(int argc, char **argv) {
  return do_vpn_server();
}
