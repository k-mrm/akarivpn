#ifndef AKARIVPN_H
#define AKARIVPN_H

#include <poll.h>

extern int terminated;

struct events {
  struct pollfd fds[16];
  int ids[16];
  int nfds;
};

#endif
