#include "ui/window_manager_state.h"
#include "session/csession.h"
#include "session/session_talk.h"
#include "game/context.h"

#include <string.h>
#include "actor/types.h"
#include "platform/clock.h"

void session_append_prohibit(struct Session *self, int aid, unsigned minutes)
{
    struct ProhibitionRecord record;
    struct String name;
    struct File file;
    struct PlatformDateTime now;
    char path[0x100], *data;
    void *actor;
    unsigned char *old = NULL;
    int count, length, actorId, aidCopy = aid;
    size_t oldSize;

    actor = game_find_actor(aid);
    if (!actor)
        return;
    actorId = ((struct GameActor *)actor)->aid;
    game_mode_get_actor_name(game_current_mode(), &name, aid);
    strcpy(record.name, string_cstr(&name));
    strcpy(record.giver, string_cstr(&self->serverName));
    record.minutes = minutes;
    record.aid = aid;
    record.actor_id = actorId;
    platform_date_time_local(&now);
    record.date = (((now.year << 8) | (short)now.month) << 8) | (short)now.day;
    record.time = (((now.hour << 8) | (short)now.minute) << 8) | (short)now.second;
    vec_push(&self->prohibitionLog, &record, sizeof record);

    file_construct(&file);
    GetCurrentDirectoryA(sizeof path, path);
    data = strstr(path, "\\data");
    count = 0;
    oldSize = 0;
    if (data)
        memcpy(path, path, data - path);
    strcat(path, "\\ProhibitionLog.gd");
    if (file_open(&file, path, 0, 1, 0)) {
        oldSize = file.size - 4;
        old = mem_alloc(oldSize);
        file_read(&file, &count, 4);
        file_read(&file, old, oldSize);
        file_close(&file);
    }
    count++;
    file_open(&file, path, 0x1001, 1, 0);
    file_write(&file, &count, 4);
    if (oldSize)
        file_write(&file, old, oldSize);
    length = (int)strlen(record.name) + 1;
    file_write(&file, &length, 4);
    file_write(&file, record.name, length);
    length = (int)strlen(record.giver) + 1;
    file_write(&file, &length, 4);
    file_write(&file, record.giver, length);
    file_write(&file, &aidCopy, 4);
    file_write(&file, &actorId, 4);
    file_write(&file, &record.minutes, 4);
    file_write(&file, &record.date, 4);
    file_write(&file, &record.time, 4);
    file_close(&file);
    if (oldSize)
        mem_free(old);
    file_destruct(&file);
    string_free(&name);
}

void session_erase_prohibit(struct Session *self, int aid, unsigned minutes)
{
    struct ProhibitionRecord *first = self->prohibitionLog.first;
    struct ProhibitionRecord *last = self->prohibitionLog.last;
    struct ProhibitionRecord *found = last, *rec;
    const char *text;

    for (rec = first; rec != last; rec++)
        if (rec->aid == aid)
            found = rec;
    if (found == last) {
        text = "채팅 로그 삭제 실패 : "
               "채팅 금지를 준 기록이 "
               "없습니다.";
    } else if (found->minutes > minutes) {
        text = "채팅 로그 삭제 안함 : "
               "마이너스 된 시간이 "
               "기존의 채팅 금지 시간보다 "
               "짧습니다.";
    } else {
        memmove(found, found + 1, (size_t)((BYTE *)last - (BYTE *)(found + 1)));
        self->prohibitionLog.last = last - 1;
        text = "채팅 로그 삭제 성공";
    }
    ui_window_mgr_message_box(&g_windowMgr, text, 0, 1, 0, 0, "메시지", 0x118, 0x78, 0);
}

int session_count_prohibits_of(struct Session *self, int aid)
{
    struct ProhibitionRecord *rec = self->prohibitionLog.first;
    int records = (int)(((BYTE *)self->prohibitionLog.last - (BYTE *)rec) / sizeof *rec);
    int n = 0, actorId, i;
    void *actor;

    actor = game_find_actor(aid);
    if (!actor)
        return -1;
    actorId = ((struct GameActor *)actor)->aid;
    self->prohibitTarget = actorId;
    for (i = 0; i < records; i++, rec++)
        if (strcmp(rec->giver, string_cstr(&self->serverName)) == 0 && rec->actor_id == actorId)
            n++;
    return n;
}
