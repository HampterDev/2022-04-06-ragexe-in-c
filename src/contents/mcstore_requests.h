#ifndef CONTENTS_MCSTORE_REQUESTS_H
#define CONTENTS_MCSTORE_REQUESTS_H

struct McStoreBackup;

int mcstore_request_load(struct McStoreBackup *self, int aid, int gid,
    const char *world, int type);
int mcstore_request_save(struct McStoreBackup *self, int type);

#endif
