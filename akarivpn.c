#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>

int terminated = 0;

void
add_event(struct events *es, int fd, void *priv) {
  es->fds[es->nfds] = (struct pollfd){
    .fd = fd,
    .events = POLLIN,
  };
  es->privs[es->nfds] = priv;

  es->nfds++;
}

void
del_event(struct events *es, int fd) {
  for(int i = 0; i < es->nfds; i++) {
    struct pollfd *pfd = &es->fds[i];
    if(pfd->fd == fd) {
      es->nfds--;

      for(int j = i; j < es->nfds; j++) {
        es->fds[j] = es->fds[j + 1];
      }
      for(int j = i; j < es->nfds; j++) {
        es->ids[j] = es->ids[j + 1];
      }
      return;
    }
  }
}

int
events_poll(struct events *es) {
  return poll(es->fds, es->nfds, 2000);
}

static void
sighandler(int sig) {
  printf("terminated: reason %d\n", sig);
  terminated = 1;
}

void
signal_init() {
  signal(SIGINT, sighandler);
  signal(SIGTERM, sighandler);
}
