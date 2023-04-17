#include <stdio.h>
#include <stdlib.h>
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
#include "tunnel.h"

struct tunnel *
tcp_tunnel_accept(int listen_fd) {
  struct tunnel *t;
  int client_fd;
  struct sockaddr_in client_sin;
  socklen_t sin_len = sizeof(client_sin);

  if((client_fd = accept(listen_fd, (struct sockaddr *)&client_sin, &sin_len)) < 0) {
    perror("accept");
    return NULL;
  }
  if(sin_len <= 0) {
    perror("wtf");
    return NULL;
  }

  t = malloc(sizeof(*t));
  if(!t)
    return NULL;
  
  t->fd = client_fd;
  return t;
}

static void
free_tunnel(struct tunnel *t) {
  free(t);
}

struct tunnel *
connect_tunnel(const char *host, const char *port, enum protocol proto) {
  int sock;
  struct tunnel *t;
  struct addrinfo hint = {0}, *res;

  hint.ai_family = PF_INET;
  hint.ai_socktype = proto == PROTO_TCP ? SOCK_STREAM : SOCK_DGRAM;

  if(getaddrinfo(host, port, &hint, &res) < 0) {
    perror("getaddrinfo");
    return NULL;
  }

  if((sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
    perror("socket");
    return NULL;
  }

  if(connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
    perror("connect");
    return NULL;
  }

  t = malloc(sizeof(*t));
  if(!t)
    return NULL;

  t->fd = sock;
  t->proto = proto;
  printf("tunnel connection done\n");

  return t;
}

void
tunnel_disconnected(struct tunnel *t) {
  printf("tunnel disconnected\n");
  free_tunnel(t);
}

int
tunnel_listen(int port) {
  int sock;
  int yes = 1;
  struct sockaddr_in sin;

  if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    return -1;
  }

  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);
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

  return sock;
}

ssize_t
tcp_tunnel_write(struct tunnel *t, unsigned char *buf, size_t n) {
  return write(t->fd, buf, n);
}

ssize_t
tcp_tunnel_read(struct tunnel *t, unsigned char *buf, size_t n) {
  return read(t->fd, buf, n);
}
