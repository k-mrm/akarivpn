#ifndef AKARI_TUNNEL_H
#define AKARI_TUNNEL_H

#include <stdio.h>
/* now TCP session only */

enum protocol {
  PROTO_TCP,
  PROTO_UDP,
};

struct tunnel {
  int fd;
  enum protocol proto;
};

struct tunnel *tcp_tunnel_accept(int listen_fd);
struct tunnel *connect_tunnel(const char *host, const char *port, enum protocol proto);
void tunnel_disconnected(struct tunnel *t);
int tunnel_listen(int port);
ssize_t tcp_tunnel_read(struct tunnel *t, unsigned char *buf, size_t n);
ssize_t tcp_tunnel_write(struct tunnel *t, unsigned char *buf, size_t n);

#endif
