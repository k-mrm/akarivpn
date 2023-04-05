#ifndef AKARI_CLIENT_H
#define AKARI_CLIENT_H

#include "tunnel.h"
#include "tuntap.h"

struct client {
  struct tunnel *tunnel;
  struct tuntap *tt;
};

#endif
