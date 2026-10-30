#ifndef CONTENTS_REPUTE_API_H
#define CONTENTS_REPUTE_API_H

struct ReputeMgr;
struct ReputeMgr *repute_mgr_instance(void);
void repute_mgr_destroy(void);
struct ReputeMgr *repute_mgr_construct(struct ReputeMgr *self);
void repute_mgr_destruct(struct ReputeMgr *self);
char repute_mgr_load(struct ReputeMgr *self);
void repute_mgr_on_changed(struct ReputeMgr *self, unsigned long long group_id,
                           unsigned long long info_id);
void repute_mgr_set_points(struct ReputeMgr *self, unsigned long long info_id,
                           long long points);
void repute_mgr_set_info_result(struct ReputeMgr *self, unsigned char flag);

#endif
