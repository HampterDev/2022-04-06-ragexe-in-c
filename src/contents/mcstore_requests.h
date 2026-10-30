#ifndef CONTENTS_MCSTORE_REQUESTS_H
#define CONTENTS_MCSTORE_REQUESTS_H

#include "core/text_view.h"

struct McStoreBackup;

int mcstore_request_load(struct McStoreBackup *self, int aid, int gid,
    struct TextView world, int type);
void mcstore_request_save(struct McStoreBackup *self, int type);

#endif
