#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>

#include "tuntap.h"

int
tun_setup(struct tuntap *tun, const char *ip, const char *netmask) {
  struct ifreq ifr;
  struct sockaddr_in *sin;
  int fd, ret = -1;

  if((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    return -1;
  strncpy(ifr.ifr_name, tun->name, sizeof(ifr.ifr_name) - 1);

  if(ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
    perror("SIOCGIFFLAGS");
    goto end;
  }
  ifr.ifr_flags |= IFF_UP;
  if(ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {
    perror("SIOCSIFFLAGS");
    goto end;
  }

  ifr.ifr_addr.sa_family = AF_INET;
  sin = (struct sockaddr_in *)&ifr.ifr_addr;
  inet_pton(AF_INET, ip, &sin->sin_addr);
  if(ioctl(fd, SIOCSIFADDR, &ifr) < 0) {
    perror("SIOCSIFADDR");
    goto end;
  }

  inet_pton(AF_INET, netmask, &sin->sin_addr);
  if(ioctl(fd, SIOCSIFNETMASK, &ifr) < 0) {
    perror("SIOCSIFNETMASK");
    goto end;
  }

  ret = 0;

end:
  close(fd);
  return ret;
}

struct tuntap *
tun_alloc(const char *devname, int tuntap) {
  int fd;
  struct ifreq ifr;
  struct tuntap *t = malloc(sizeof(*t));
  if(!t)
    return NULL;

  if((fd = open("/dev/net/tun", O_RDWR)) < 0) {
    perror("open /dev/net/tun failed");
    return NULL;
  }

  ifr.ifr_flags = tuntap | IFF_NO_PI;
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
tun_write(struct tuntap *t, unsigned char *buf, size_t n) {
  return write(t->fd, buf, n);
}

ssize_t
tun_read(struct tuntap *t, unsigned char *buf, size_t n) {
  return read(t->fd, buf, n);
}
