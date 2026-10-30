#ifndef UI_SOCIAL_ENTRY_QUEUE_STATE_H
#define UI_SOCIAL_ENTRY_QUEUE_STATE_H
#include <stddef.h>

struct ClientLua;
typedef struct lua_State lua_State;

enum { ENTRY_QUEUE_WND_ID = 0x9d, ENTRY_QUEUE_STANDBY_WND_ID = 0xd1,
       ENTRY_QUEUE_REQUEST_WND_ID = 0xd2, ENTRY_QUEUE_HELP_WND_ID = 0xd3 };
enum { ENTRY_APPLY_ALONE = 1, ENTRY_APPLY_PARTY = 2, ENTRY_APPLY_GUILD = 4,
       ENTRY_ANSWER_ENTER = 1, ENTRY_ANSWER_DECLINE = 2 };

struct EntryQueueInfo {
    int id;
    char name[260], title[260];
    int member_limit, enemy_limit, option_flags;
    unsigned char kind;
    int kind_arg1, kind_arg2;
    char text_row4[260], text_row5[260], text_row6[260], text_row1[260];
    char picture[260];
};

struct EntryQueueMgr {
    char queue_name[24];
    char lobby_name[24];
    struct EntryQueueInfo *current;
    unsigned char in_queue;
};
extern struct EntryQueueMgr *g_entryQueueMgr;

size_t entry_queue_count(void);
struct EntryQueueInfo *entry_queue_at(size_t index);
struct EntryQueueInfo *entry_queue_find(unsigned id);
int entry_queue_catalog_load(struct ClientLua *lua);
void entry_queue_catalog_clear(void);
int entry_queue_lua_add(lua_State *L);

unsigned char entry_queue_mgr_create(void);
void entry_queue_mgr_release(void);
void entry_queue_mgr_apply(struct EntryQueueMgr *self, short type, const char *name);
void entry_queue_mgr_cancel(struct EntryQueueMgr *self);
void entry_queue_mgr_answer(struct EntryQueueMgr *self, int answer);
void entry_queue_mgr_request_ranking(struct EntryQueueMgr *self);
void entry_queue_mgr_on_apply_result(struct EntryQueueMgr *self, const unsigned char *packet);
void entry_queue_mgr_on_cancel_result(struct EntryQueueMgr *self, const unsigned char *packet);
void entry_queue_mgr_on_leave(struct EntryQueueMgr *self, const unsigned char *packet);
void entry_queue_mgr_on_ignored(struct EntryQueueMgr *self, const unsigned char *packet);
void entry_queue_mgr_on_status(struct EntryQueueMgr *self, const unsigned char *packet);
void entry_queue_mgr_on_request(struct EntryQueueMgr *self, const unsigned char *packet);
void entry_queue_mgr_on_answer_result(struct EntryQueueMgr *self, const unsigned char *packet);
void entry_queue_mgr_on_forward(struct EntryQueueMgr *self, const unsigned char *packet);
#endif
