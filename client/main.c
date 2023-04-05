#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <linux/if_packet.h>
#include <getopt.h>

#include "akarivpn.h"
#include "netif.h"
#include "tuntap.h"

extern int terminated;

struct vpn_client {
  struct tuntap *tun;
  int server_fd;
  struct pollfd fds[2];
  int nfds;
};

static int
clientcore(struct vpn_client *cli) {
  int nready = poll(cli->fds, cli->nfds, 2000);
  ssize_t size, wsize;
  unsigned char buf[2048];

  if(nready < 0)
    return -1;

  for(int i = 0; i < cli->nfds && nready; i++) {
    struct pollfd *fd = &cli->fds[i];
    if(fd->revents & POLLIN) {
      nready--;

      if(i == 0) {    // from tap
        if((size = tun_read(cli->tun, buf, sizeof(buf))) <= 0) {
          perror("tunread");
          return -1;
        }

        if((wsize = write(cli->server_fd, buf, size)) <= 0) {
          perror("disconnected?");
          return -1;
        }
      } else {        // from server
        if((size = read(cli->server_fd, buf, sizeof(buf))) <= 0) {
          printf("server disconnected\n");
          return -1;
        }

        if((wsize = tun_write(cli->tun, buf, size)) <= 0) {
          perror("tunwrite");
          return -1;
        }
      }
    }
  }

  return 0;
}

static int
clientloop(struct vpn_client *cli) {
  cli->fds[0] = (struct pollfd){
    .fd = cli->tun->fd,
    .events = POLLIN,
  };
  cli->fds[1] = (struct pollfd){
    .fd = cli->server_fd,
    .events = POLLIN,
  };
  cli->nfds = 2;

  printf("fd tun %d serv %d\n", cli->tun->fd, cli->server_fd);
  printf("tap %s\n", cli->tun->name);

  while(!terminated && clientcore(cli) == 0)
    ;

  return 0;
}

static int
do_vpn_client(char *host, char *port, char *myip) {
  struct vpn_client client;
  struct tunnel *tunnel;

  if((client.tun = tun_alloc("tap114514", IFF_TAP)) == NULL) {
  // if((client.tun = tun_alloc("tun19", IFF_TUN)) == NULL) {
    printf("wtf\n");
    return -1;
  }
  tun_setup(client.tun, myip, "255.255.255.0");

  tunnel = connect_tunnel(host, port, PROTO_TCP);
  if(!tunnel)
    return -1;

  return clientloop(&client);
}

int
main(int argc, char **argv) {
  int opt, port = -1;
  char *serv_ip = NULL, *myip = NULL;

  while((opt = getopt(argc, argv, "i:s:p:")) != -1) {
    switch(opt) {
      case 'i':
        myip = optarg;
        break;
      case 's':
        serv_ip = optarg;
        break;
      case 'p':
        port = atoi(optarg);
        break;
      case '?':
        printf("parse error\n");
        return -1;
      default:
        break;
    }
  }

  if(!serv_ip) {
    printf("server ip?\n");
    return -1;
  }
  if(!myip) {
    puts("?");
    return -1;
  }
  if(port < 0)
    port = 1145;    // default port

  signal_init();

  return do_vpn_client(serv_ip, port, myip);
}
