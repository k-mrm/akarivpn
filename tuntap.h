#ifndef AKARI_TUNTAP_H
#define AKARI_TUNTAP_H

#include <stdio.h>
#include <sys/ioctl.h>
#include <net/if.h> 
#include <linux/if_tun.h>

struct tuntap {
  char name[IFNAMSIZ];
  struct ifreq ifr;
  int fd;
};

struct tuntap *tun_alloc(const char *devname, int tuntap);
int tun_setup(struct tuntap *tun, const char *ip, const char *submask);
ssize_t tun_write(struct tuntap *t, unsigned char *buf, size_t n);
ssize_t tun_read(struct tuntap *t, unsigned char *buf, size_t n);

#endif
