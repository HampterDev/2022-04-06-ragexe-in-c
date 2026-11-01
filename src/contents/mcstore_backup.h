#ifndef CONTENTS_MCSTORE_BACKUP_H
#define CONTENTS_MCSTORE_BACKUP_H

#include "contents/mcstore_backup_types.h"
#include "contents/mcstore_requests.h"

struct JsonTree;
struct UIMerchantShopMakeWnd;

void mcstore_init(struct McStoreBackup *self);
void mcstore_destroy(struct McStoreBackup *self);
void mcstore_reset(struct McStoreBackup *self);
void mcstore_clear_items(struct McStoreBackup *self);
void mcstore_restore_shop(struct McStoreBackup *self, char mode, int count);

int mcstore_read_items(struct McStoreBackup *self, const struct JsonTree *data);
int mcstore_read_item(struct McStoreItem *item, const struct JsonTree *object);
int mcstore_write_items(struct McStoreBackup *self, struct JsonTree *out);

void ui_merchant_shop_make_wnd_apply_backup(struct UIMerchantShopMakeWnd *window);

#endif
