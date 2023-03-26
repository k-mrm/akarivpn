#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

int terminated = 0;

void
akrvpn_err(const char *msg) {
  fprintf(stderr, "[ERR] %s\n", msg);
  exit(1);
}

void
akrvpn_info(const char *msg) {
  fprintf(stderr, "[INFO] %s\n", msg);
}

void
akrvpn_dbg(const char *msg) {
  ;
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
