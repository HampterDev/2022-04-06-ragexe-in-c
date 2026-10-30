#ifndef ASYNCWORK_WORK_POOL_H
#define ASYNCWORK_WORK_POOL_H

struct AsyncWork;
struct WorkPoolMgr;

struct WorkPoolMgr *work_pool_mgr_create(void);
void work_pool_mgr_destroy(struct WorkPoolMgr *manager);
struct AsyncWork *work_pool_mgr_acquire(struct WorkPoolMgr *manager, const char *name);
void work_pool_mgr_release(struct WorkPoolMgr *manager, struct AsyncWork *work);

#endif
