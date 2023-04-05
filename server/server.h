#ifndef AKARIVPN_SERVER_H
#define AKARIVPN_SERVER_H

#include <poll.h>

#include "akarivpn.h"
#include "netif.h"
#include "tuntap.h"
#include "tunnel.h"

struct vpn_server {
  struct events events;
  int listen_fd;
  struct tunnel *clients[16];
  struct tuntap *tun;
};

#endif
