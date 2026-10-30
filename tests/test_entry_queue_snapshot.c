#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui/social/entry_queue_state.h"
#include "ui/social/entry_queue_snapshot.h"
#include "fixtures/entry_queue_wire.h"

static int allocations, attempts, fail_at;
static void *resize(void *old, size_t size)
{
    void *p;
    if (++attempts == fail_at) return NULL;
    p = realloc(old, size); assert(p); if (!old) ++allocations; return p;
}
static void release(void *p) { if (p) { --allocations; free(p); } }
#define realloc resize
#define free release
#include "../src/replay/snapshot.c"
#undef realloc
#undef free
#include "../src/ui/social/entry_queue_snapshot.c"
#include "../src/ui/social/entry_queue_status.c"

void *mem_alloc(size_t size) { void *p=malloc(size);assert(p);++allocations;return p; }
void mem_free(void *p)
{ release(p); }
void fatal_error(const char *message) { (void)message;abort(); }
static struct EntryQueueInfo queue;
static struct EntryQueueMgr manager;
struct EntryQueueMgr *g_entryQueueMgr=&manager;
static struct UIEntryQueueStandByWnd standby;
static struct UIWindow choice;
static int windows=1, selected=3, selected_updates, clears, lines;
static char rendered[3][260];
struct EntryQueueInfo *entry_queue_find(unsigned id) { return id==7 ? &queue : NULL; }
struct UIWindow *ui_window_find(int id)
{
    if(!windows)return NULL;
    return id==ENTRY_QUEUE_WND_ID ? &choice : id==ENTRY_QUEUE_STANDBY_WND_ID ? &standby.frame.base : NULL;
}
void *object_as_type(void *object,enum ObjectType type)
{
    if(object==&choice && type==OBJECT_ENTRY_QUEUE_WINDOW)return object;
    if(object==&standby && type==OBJECT_ENTRY_QUEUE_STANDBY_WINDOW)return object;
    return NULL;
}
int ui_entry_queue_wnd_get_selected(struct UIEntryQueueWnd *self) { assert(self==(void *)&choice);return selected; }
int ui_window_send(struct UIWindow *wnd,struct UIWindow *sender,int msg,int a,int b,int c,int d)
{
    assert(wnd==&choice && !sender && msg==0x87 && !a && !c && !d);
    selected=b;++selected_updates;return 0;
}
char *msg_str(int id)
{
    if(id==MSG_ENTRY_STANDBY_QUEUE)return "Queue: %s";
    if(id==MSG_ENTRY_STANDBY_ARG1)return "Limit: %d";
    assert(id==MSG_ENTRY_STANDBY_ARG2);return "Position: %d";
}
static void clear_lines(struct UIListBox *self) { (void)self; ++clears;lines=0; }
static int add_line(struct UIWindow *self,struct UIWindow *sender,int msg,int a,int b,int c,int d)
{
    (void)self;assert(!sender && msg==UI_MSG_ADD_TEXT && b==0x26 && !c && !d && lines<3);
    strcpy_s(rendered[lines++],260,(const char *)a);return 0;
}
static void reset(void)
{
    memset(&manager,0,sizeof manager);manager.current=&queue;manager.in_queue=1;
    strcpy_s(manager.queue_name,24,"queue");strcpy_s(manager.lobby_name,24,"reply");
    ui_entry_queue_stand_by_wnd_set_status(&standby,"waiting",10,20); selected=3;selected_updates=0;
}
static void invalid(const void *bytes,size_t size)
{
    struct EntryQueueMgr before=manager;
    int old_updates=selected_updates,old_allocations=allocations,old_clears=clears;
    assert(entry_queue_snapshot_load(&manager,bytes,size)==-1);
    assert(!memcmp(&manager,&before,sizeof manager) && selected_updates==old_updates);
    assert(allocations==old_allocations && clears==old_clears);
}
static struct ReplayBuffer record(unsigned tag,const void *bytes,size_t size)
{
    struct SnapshotWriter writer={0};struct ReplayBuffer out={0};
    snapshot_write(&writer,0x1964,NULL,0);snapshot_write(&writer,(unsigned short)tag,bytes,size);
    snapshot_write(&writer,0x1965,NULL,0);assert(snapshot_finish(&writer,&out));return out;
}
int main(void)
{
    struct ReplayBuffer out={0},bad;struct SnapshotWriter writer={0};struct EntryQueueStatusView status;
    const struct UIListBoxOps ops={.base={.send_msg=add_line},.clear_items=clear_lines};
    struct UIListBox viewer={0};
    unsigned char data[1000];size_t i;int value=99,total,live;
    queue.id=7;string_init(&standby.queue);
    viewer.base.ops=&ops.base;standby.text=&viewer.base;
    reset();assert(lines==3 && !strcmp(rendered[0],"Queue: waiting") && !strcmp(rendered[1],"Limit: 10"));
    assert(entry_queue_snapshot_save(&manager,&out));
    assert(out.size==sizeof expected_queue && !memcmp(out.data,expected_queue,out.size));
    for(i=0;i<sizeof expected_queue;++i)invalid(expected_queue,i);
    invalid(NULL,0);assert(!entry_queue_snapshot_save(NULL,&out));
    assert(entry_queue_snapshot_load(NULL,expected_queue,sizeof expected_queue)==-1);
    memset(&manager,0,sizeof manager);selected=-1;
    assert(entry_queue_snapshot_load(&manager,expected_queue,sizeof expected_queue)==sizeof expected_queue);
    assert(manager.current==&queue && manager.in_queue==1 && !strcmp(manager.queue_name,"queue"));
    assert(!strcmp(manager.lobby_name,"reply") && selected==3 && selected_updates==1);
    entry_queue_standby_status(&standby,&status);assert(status.arg1==10 && status.arg2==20 && status.size==7 && !memcmp(status.text,"waiting",7));
    for(i=0x196f;i<=0x1976;++i) {
        size_t width=i==0x1970 ? 2 : (i==0x1971 || i==0x1972) ? 25 : 3;
        if(i==0x1974)continue;
        memset(data,0,sizeof data);bad=record((unsigned)i,data,width);invalid(bad.data,bad.size);replay_buffer_destroy(&bad);
    }
    bad=record(0x1964,NULL,0);invalid(bad.data,bad.size);replay_buffer_destroy(&bad);
    bad=record(0x1965,data,1);invalid(bad.data,bad.size);replay_buffer_destroy(&bad);
    bad=record(0x196f,&value,4);assert(entry_queue_snapshot_load(&manager,bad.data,bad.size)>0 && manager.current==&queue);replay_buffer_destroy(&bad);
    memset(data,'x',24);bad=record(0x1971,data,24);
    assert(entry_queue_snapshot_load(&manager,bad.data,bad.size)>0 && !memcmp(manager.queue_name,data,24));replay_buffer_destroy(&bad);
    assert(entry_queue_snapshot_save(&manager,&out));
    snapshot_write(&writer,0x1964,NULL,0);snapshot_write(&writer,0x1974,"new",3);
    snapshot_write(&writer,0xffff,"unknown",7);snapshot_write(&writer,0x1976,&value,4);
    snapshot_write(&writer,0x1965,NULL,0);bad.data=NULL;bad.size=0;assert(snapshot_finish(&writer,&bad));
    assert(entry_queue_snapshot_load(&manager,bad.data,bad.size)>0 && standby.arg1==0 && standby.arg2==99);
    replay_buffer_destroy(&bad);bad=record(0x1974,"first\0suffix",12);
    assert(entry_queue_snapshot_load(&manager,bad.data,bad.size)>0 && standby.queue.size==5);replay_buffer_destroy(&bad);
    memset(data,'z',sizeof data);bad=record(0x1974,data,sizeof data);
    assert(entry_queue_snapshot_load(&manager,bad.data,bad.size)>0 && standby.queue.size==sizeof data);
    assert(strlen(rendered[0])==259 && lines==3);replay_buffer_destroy(&bad);
    attempts=0;assert(entry_queue_snapshot_save(&manager,&out));total=attempts;live=allocations;assert(total>=2);
    for(i=1;i<=(size_t)total;++i) {
        unsigned char *old=out.data;size_t size=out.size;attempts=0;fail_at=(int)i;
        assert(!entry_queue_snapshot_save(&manager,&out));fail_at=0;
        assert(out.data==old && out.size==size && allocations==live);
    }
    windows=0;selected_updates=0;
    assert(entry_queue_snapshot_load(&manager,expected_queue,sizeof expected_queue)>0 && !selected_updates);
    manager.current=NULL;memset(manager.queue_name,0,24);memset(manager.lobby_name,0,24);
    assert(entry_queue_snapshot_save(&manager,&out) && out.size==19);
    replay_buffer_destroy(&out);replay_buffer_destroy(&out);string_free(&standby.queue);
    assert(!allocations);
    puts("Entry queue snapshots: frozen legacy bytes, every truncation, exact widths, deferred UI, bounded real window text, allocation rollback and cleanup passed");
    return 0;
}
