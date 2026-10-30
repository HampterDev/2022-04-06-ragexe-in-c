#ifndef ASYNCWORK_WORK_MANAGER_H
#define ASYNCWORK_WORK_MANAGER_H

struct AsyncWorkMgr;
extern struct AsyncWorkMgr *g_asyncWorkMgr;

int async_work_mgr_create(int thread_count);
void async_work_mgr_destroy(void);
int async_work_mgr_start(void);
void async_work_mgr_stop(void);
void async_work_mgr_process_responses(unsigned count);

#endif
