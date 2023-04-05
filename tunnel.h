#ifndef AKARI_TUNNEL_H
#define AKARI_TUNNEL_H

/* now TCP session only */

enum protocol {
  PROTO_TCP,
  PROTO_UDP,
};

struct tunnel {
  int fd;
  enum protocol proto;
};

#endif
