#ifndef AKARIVPN_FDB_H
#define AKARIVPN_FDB_H

#include <stdint.h>

struct fdbent {
  uint8_t mac[6];
  struct tunnel *client;
};

struct fdb {
  struct fdbent ents[256];
  int nents;
};

struct tunnel *search(struct fdb *fdb, uint8_t *mac);
int learn(struct fdb *fdb, struct tunnel *t, uint8_t *mac);


#endif
