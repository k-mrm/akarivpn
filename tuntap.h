#ifndef AKARI_TUNTAP_H
#define AKARI_TUNTAP_H

#include <stdio.h>
#include <sys/ioctl.h>
#include <net/if.h> 

struct tunnel {
  char name[IFNAMSIZ];
  int fd;
};

struct tunnel *tun_alloc(const char *devname);
ssize_t tun_write(struct tunnel *t, char *buf, size_t n);
ssize_t tun_read(struct tunnel *t, char *buf, size_t n);

#endif
