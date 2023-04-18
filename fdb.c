#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "fdb.h"

int
learn(struct fdb *fdb, struct tunnel *t, uint8_t *mac) {
  struct fdbent *ent;

  if(search(fdb, mac))
    return 0;
  if(fdb->nents == 256)
    return -1;

  ent = &fdb->ents[fdb->nents++];

  memcpy(ent->mac, mac, 6);
  ent->client = t;

  return 0;
}

struct tunnel *
search(struct fdb *fdb, uint8_t *mac) {
  for(struct fdbent *ent = fdb->ents; ent < &fdb->ents[fdb->nents]; ent++) {
    if(memcmp(ent->mac, mac, 6) == 0)
      return ent->client;
  }

  return NULL;
}
