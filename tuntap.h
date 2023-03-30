#ifndef AKARI_TUNTAP_H
#define AKARI_TUNTAP_H

#include <stdio.h>
#include <sys/ioctl.h>
#include <net/if.h> 
#include <linux/if_tun.h>

struct tunnel {
  char name[IFNAMSIZ];
  struct ifreq ifr;
  int fd;
};

struct tunnel *tun_alloc(const char *devname, int tuntap);
int tun_up(struct tunnel *tun);
ssize_t tun_write(struct tunnel *t, unsigned char *buf, size_t n);
ssize_t tun_read(struct tunnel *t, unsigned char *buf, size_t n);

#endif
