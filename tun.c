#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/if_packet.h>
#include <linux/if_tun.h>

#include "tuntap.h"

struct tunnel *
tun_alloc(const char *devname) {
  int fd;
  struct ifreq ifr = {0};
  struct tunnel *t = malloc(sizeof(*t));
  if(!t)
    return NULL;

  if((fd = open("/dev/net/tun", O_RDWR)) < 0) {
    perror("open /dev/net/tun failed");
    return NULL;
  }

  ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
  strncpy(ifr.ifr_name, devname, sizeof(ifr.ifr_name) - 1);
  if(ioctl(fd, TUNSETIFF, &ifr) < 0) {
    perror("TUNSETIFF");
    return NULL;
  }

  strncpy(t->name, ifr.ifr_name, sizeof(ifr.ifr_name) - 1);
  t->fd = fd;

  return t;
}

ssize_t
tun_write(struct tunnel *t, char *buf, size_t n) {
  return write(t->fd, buf, n);
}

ssize_t
tun_read(struct tunnel *t, char *buf, size_t n) {
  return read(t->fd, buf, n);
}
