#ifndef ASYNCWORK_WORK_MANAGER_INTERNAL_H
#define ASYNCWORK_WORK_MANAGER_INTERNAL_H

#include "web/work_manager.h"
#include "web/work.h"

struct AsyncWork *async_work_mgr_acquire(struct AsyncWorkMgr *manager, const char *name);
void async_work_mgr_cancel(struct AsyncWorkMgr *manager, struct AsyncWork *work);
int async_work_mgr_submit(struct AsyncWorkMgr *manager, struct AsyncWork *work);

#endif
