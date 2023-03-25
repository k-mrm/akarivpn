#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
