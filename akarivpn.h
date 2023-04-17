#ifndef AKARIVPN_H
#define AKARIVPN_H

#include <poll.h>

extern int terminated;

struct events {
  struct pollfd fds[16];
  void *privs[16];
  int nfds;
};

struct revent {
  struct pollfd fd;
  void *priv;
};

int events_poll_pollin(struct events *es, int timeout, struct revent *revs);
void del_event(struct events *es, int fd);
void add_event(struct events *es, int fd, void *priv);

#endif
