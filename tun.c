#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "tuntap.h"

struct tunnel *
tun_alloc(const char *devname, int tuntap) {
  int fd;
  struct tunnel *t = malloc(sizeof(*t));
  if(!t)
    return NULL;

  if((fd = open("/dev/net/tun", O_RDWR)) < 0) {
    perror("open /dev/net/tun failed");
    return NULL;
  }

  t->ifr.ifr_flags = tuntap | IFF_NO_PI;
  strncpy(t->ifr.ifr_name, devname, sizeof(t->ifr.ifr_name) - 1);
  if(ioctl(fd, TUNSETIFF, &t->ifr) < 0) {
    perror("TUNSETIFF");
    return NULL;
  }

  strncpy(t->name, t->ifr.ifr_name, sizeof(t->ifr.ifr_name) - 1);
  t->fd = fd;

  return t;
}

ssize_t
tun_write(struct tunnel *t, unsigned char *buf, size_t n) {
  return write(t->fd, buf, n);
}

ssize_t
tun_read(struct tunnel *t, unsigned char *buf, size_t n) {
  return read(t->fd, buf, n);
}
