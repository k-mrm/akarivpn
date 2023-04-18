#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <netdb.h>
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
#include "fdb.h"

extern int terminated;

struct vpn_server {
  struct events events;
  struct tuntap *tt;
  struct tunnel *clients[64];
  int nclients;
  struct fdb fdb;
};

#define EV_NULL         0
#define EV_LISTEN       1
#define EV_TUNTAP       2

static void
addclient(struct vpn_server *s, struct tunnel *t) {
  s->clients[s->nclients] = t;
  s->nclients++;
}

static void
delclient(struct vpn_server *s, struct tunnel *t) {
  for(int i = 0; i < s->nclients; i++) {
    if(s->clients[i] == t) {
      s->nclients--;
      for(int j = i; j < s->nclients; j++) {
        s->clients[j] = s->clients[j + 1];
      }
      return;
    }
  }
}

static int
client_accept(struct vpn_server *serv, int listen_fd) {
  struct tunnel *t = tcp_tunnel_accept(listen_fd);
  if(!t)
    return -1;

  add_event(&serv->events, t->fd, t);
  addclient(serv, t);

  return 0;
}

static void
broadcast_to_clients(struct vpn_server *serv, unsigned char *buf, size_t size) {
  for(int i = 0; i < serv->nclients; i++) {
    struct tunnel *t = serv->clients[i];

    if(tcp_tunnel_write(t, buf, size) <= 0) {
      printf("tunnel write?\n");
    }
  }
}

static bool
is_broadcast_addr(uint8_t *mac) {
  static uint8_t baddr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

  return !memcmp(mac, baddr, 6);
}

static int
route(struct vpn_server *serv, unsigned char *buf, ssize_t size) {
  struct ether_header *eth = (struct ether_header *)buf;
  uint8_t *dst = eth->ether_dhost;

  if(is_broadcast_addr(dst)) {
    broadcast_to_clients(serv, buf, size);
  } else {
    struct tunnel *t = search(&serv->fdb, dst);
    if(t) {
      puts("gomen");
    } else {
      broadcast_to_clients(serv, buf, size);
    }
  }

  return 0;
}

static int
servercore(struct vpn_server *serv) {
  ssize_t size;
  unsigned char buf[2048];
  int nready;
  struct revent revs[16];

  if((nready = events_poll_pollin(&serv->events, 2000, revs)) < 0)
    return -1;

  for(int i = 0; i < nready; i++) {
    struct pollfd *pfd = &revs[i].fd;
    void *priv = revs[i].priv;
    struct tunnel *tunnel;
    int listen_fd, tun_fd;

    switch((size_t)priv) {
      case EV_LISTEN:
        listen_fd = pfd->fd;

        if(client_accept(serv, listen_fd) < 0)
          return -1;
        break;
      case EV_TUNTAP:
        if((size = tun_read(serv->tt, buf, sizeof(buf))) <= 0)
          return -1;
        if(route(serv, buf, size) < 0)
          return -1;
        break;
      case EV_NULL:
        printf("error");
        return -1;
      default:
        tunnel = priv; 

        if((size = tcp_tunnel_read(tunnel, buf, sizeof(buf))) <= 0) {
          tunnel_disconnected(tunnel);
          delclient(serv, tunnel);
          return 0;
        }
        l2packet_dump(buf, size);
        if(tun_write(serv->tt, buf, size) <= 0)
          return -1;
        break;
    }
  }

  return 0;
}

static void
vpn_server_close(struct vpn_server *serv) {
  ;
}

static int
serverloop(struct vpn_server *serv, int listen_fd) {
  while(!terminated) {
    printf("waiting connection...\n");
    if(client_accept(serv, listen_fd) < 0)
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

  if((serv.tt = tun_alloc("tap1919", IFF_TAP)) == NULL) {
    printf("wtf\n");
    return -1;
  }
  tun_setup(serv.tt, "192.168.1.1", "255.255.255.0");

  add_event(&serv.events, listen_fd, (void *)EV_LISTEN);
  add_event(&serv.events, serv.tt->fd, (void *)EV_TUNTAP);

  return serverloop(&serv, listen_fd);
}

int
main(int argc, char **argv) {
  return do_vpn_server();
}
