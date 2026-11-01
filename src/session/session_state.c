#include "render/renderer.h"
#include "session/character_name.h"
#include "contents/guild_info.h"
#include "contents/guild_emblem_name.h"
#include "session/csession.h"
#include "session/session_jobs.h"
#include "session/session_names.h"
#include "session/session_state.h"
#include "session/session_windows.h"

#include <stdio.h>
#include <io.h>
#include <direct.h>
#include <stdlib.h>
#include <string.h>
#include <mmsystem.h>
#include "render/render_api.h"
#include "platform/language.h"
#include "core/file_util.h"
#include "platform/clientinfo.h"
#include "ui/wndmgr/wndmgr.h"
#include "core/app.h"
#include "game/store_search_items.h"

void session_clear_shorten_item_name(struct Session *self)
{
    int i;
    for (i = 0; i < 9; i++)
        string_assign_cstr(&self->shortenSkillNames[i], "nothing");
}

void session_clear_plate_records(struct Session *self)
{
    list_clear(&self->plateRecords, NULL);
}

void session_clear_stat_bonuses(struct Session *self)
{
    self->statBonus[0] = 0;
    self->statBonus[4] = 0;
    self->statBonus[5] = 0;
    self->statBonus[1] = 0;
    self->statBonus[2] = 0;
    self->statBonus[3] = 0;
}

BYTE session_cash_shop_goods_empty(struct Session *self)
{
    int i;
    for (i = 0; i < 11; i++)
        if (self->cashShopGoods[i].size != 0)
            return 0;
    return 1;
}

int session_get_homun_skill_use_level(struct Session *self, int skillId)
{
    int i = skillId - 8000;
    if (i <= -1)
        return 0;
    return ((int *)self->homunSkillUseLevels.first)[i];
}

void session_set_homun_skill_use_level(struct Session *self, int skillId, int value)
{
    int i = skillId - 8000;
    if (i > -1)
        ((int *)self->homunSkillUseLevels.first)[i] = value;
}

void session_set_merc_skill_use_level(struct Session *self, int skillId, int value)
{
    int i = skillId - 0x2008;
    if (i > -1)
        ((int *)self->merSkillUseLevels.first)[i] = value;
}

void scale_rect_to_screen(RECT *r)
{
    float sx, sy;
    const struct Renderer *renderer;

    if (!r)
        return;
    renderer = g_renderer;
    sx = (float)renderer->width / 1280.0f;
    sy = (float)(renderer->height - 0x11) * 0.0009765625f;
    r->left = (int)((double)((float)r->left * sx) + 0.5);
    r->top = (int)((double)((float)r->top * sy) + 0.5) + 0x11;
    r->right = (int)((double)((float)r->right * sx) + 0.5);
    r->bottom = (int)((double)((float)r->bottom * sy) + 0.5) + 0x11;
}

int session_get_merc_skill_use_level(struct Session *self, int skillId)
{
    int i = skillId - 0x2008;
    if (i <= -1)
        return 0;
    return ((int *)self->merSkillUseLevels.first)[i];
}

struct PlateRecord *session_get_plate_record(struct Session *self,
                                                 struct PlateRecord *out, int memberId)
{
    struct ListNode *node;

    for (node = self->plateRecords.head->next; node != self->plateRecords.head; node = node->next) {
        if (LIST_VALUE(node, struct PlateRecord)->id == memberId) {
            *out = *LIST_VALUE(node, struct PlateRecord);
            return out;
        }
    }
    out->found = 0;
    return out;
}

int session_get_skill_table_value(struct Session *self, int skillId)
{
    int i;

    if ((unsigned)(skillId - 1) <= 0x157a)
        return ((int *)self->skills.chosen_levels.first)[skillId];
    if (skillId > 10000) {
        i = skillId - 10000;
        if (i <= -1)
            return 0;
        return ((int *)self->guildSkillUseLevels.first)[i];
    }
    if (session_is_homunculus_skill(self, skillId) == 0)
        return 0;
    i = skillId - 8000;
    if (i <= -1)
        return 0;
    return ((int *)self->homunSkillUseLevels.first)[i];
}

int session_get_transparent_item_alpha(struct Session *self, int key)
{
    static const struct Pulse missing;
    const struct Pulse *p = tree_find(&self->transparentItemPulses, &key, tree_less_int);
    int range, base, half, value;
    DWORD t;

    if (!p)
        p = &missing;

    range = p->hi - p->lo;
    if (range < 0)
        range = -range;
    if (range == 0)
        return p->hi;
    base = p->hi >= p->lo ? p->lo : p->hi;
    half = (int)((float)(int)p->period * 0.5f);
    t = timeGetTime() % p->period;
    if ((int)t > half)
        t = p->period - t;
    value = (int)((float)(int)t / (float)half * (float)range + (float)base);
    if (value < 0)
        value = 0;
    if (value > 0xff)
        value = 0xff;
    return value;
}

void session_reset_store_search(struct Session *self)
{
    store_search_items_destroy(self->storeSearchResults.first, self->storeSearchResults.last);
    vec_clear(&self->storeSearchResults);
    self->storeSearchesLeft = 0;
    self->storeSearchOpenOnEnter = 0;
    self->storeSearchHasNextPage = 0;
    self->storeSearchItemLimit = 0;
    self->store_search_pos_x = -1;
    self->store_search_pos_y = -1;
    self->storeSearchClickedAid = -1;
}

int session_is_admin_option(struct Session *self, int what)
{
    if (clientinfo_is_admin_aid(self->ownAid) && what == 7)
        return 1;
    return 0;
}

BYTE session_is_screen_flipped(struct Session *self)
{
    return g_wobbleFramesLeft != 0 && g_wobbleMode == 2;
}

void session_push_ping_sample(struct Session *self, unsigned value)
{
    struct List *list = &self->pingSamples;
    struct ListNode *node;
    unsigned sum = 0;

    list_push_back(list, &value, sizeof value);
    while (list->size > 3)
        list_erase(list, list->head->next, NULL);
    for (node = list->head->next; node != list->head; node = node->next)
        sum += *LIST_VALUE(node, unsigned);
    self->averagePing = sum / list->size;
}

void session_erase_plate_record(struct Session *self, int value)
{
    struct ListNode *node;

    for (node = self->plateRecords.head->next; node != self->plateRecords.head; node = node->next)
        if (LIST_VALUE(node, struct PlateRecord)->id == value) {
            list_erase(&self->plateRecords, node, NULL);
            return;
        }
}

void session_save_mini_party_info(struct Session *self)
{
    struct String text;
    struct ListNode *node;
    FILE *f;

    string_init(&text);
    for (node = self->plateRecords.head->next; node != self->plateRecords.head; node = node->next) {
        const struct PlateRecord *v = LIST_VALUE(node, struct PlateRecord);
        char line[0x200];

        memset(line, 0, sizeof line);
        sprintf_s(line, 0x200, "{%s,%d,%d,%d}\n", session_character_name(&g_session1), v->found, v->id, v->cell);
        string_append_cstr(&text, line);
    }
    f = fopen("savedata\\MiniPartyInfo.lua", "w");
    if (f) {
        int (*print)(FILE *, const char *, ...) = fprintf;
        print(f, string_cstr(&text));
        fclose(f);
    }
    string_free(&text);
}

static void destroy_string(void *value) { string_free(value); }

void session_save_emblem(struct Session *self, const void *data, size_t size,
                                    int guildId, int version)
{
    char dir[0x100], name[0x100], path[0x100];
    struct List files;
    struct ListNode *node;
    FILE *f;

    if (guildId == g_guildInfo.guild_id)
        guild_mgr_set_emblem_version(g_guildMgr, guildId, version);
    sprintf(dir, "%s\\_tmpEmblem\\", g_clientDir);
    if (_access(dir, 0) != 0 && _mkdir(dir) == -1)
        return;
    sprintf(name, "%s_%d_*.ebm", string_cstr(&self->worldName), guildId);
    list_init(&files);
    dir_list_names_list(&files, dir, name);
    for (node = files.head->next; node != files.head; node = node->next) {
        sprintf(name, "%s%s", dir, string_cstr(LIST_VALUE(node, struct String)));
        _chmod(name, 0x80);
        _unlink(name);
    }
    {
        struct String name;

        guild_emblem_name(&name, &self->worldName, (unsigned)guildId, version);
        snprintf(path, sizeof path, "%s%s", dir, string_cstr(&name));
        string_free(&name);
    }
    f = fopen(path, "wb");
    if (f) {
        fwrite(data, size, 1, f);
        fclose(f);
    }
    list_free(&files, destroy_string);
}

void session_add_plate_record(struct Session *self, int memberId, int cell)
{
    struct PlateRecord v;

    v.found = 1;
    v.id = memberId;
    v.cell = cell;
    list_push_back(&self->plateRecords, &v, sizeof v);
}

void session_set_plate_cell(struct Session *self, int memberId, int value)
{
    struct ListNode *node;

    for (node = self->plateRecords.head->next; node != self->plateRecords.head; node = node->next)
        if (LIST_VALUE(node, struct PlateRecord)->id == memberId) {
            LIST_VALUE(node, struct PlateRecord)->cell = value;
            return;
        }
}

void session_set_server_time(struct Session *self, DWORD serverTime)
{
    self->serverTimeOffset = timeGetTime() - serverTime;
    self->serverTimeLagCount = 0;
}

void session_sync_server_time(struct Session *self, DWORD serverTime)
{
    DWORD now = timeGetTime() - 72 - self->serverTimeOffset;

    if (now + 72 < serverTime) {
        self->serverTimeOffset = timeGetTime() - serverTime;
    } else if (now + 72 > serverTime + 0x90) {
        if (++self->serverTimeLagCount < 4)
            return;
        self->serverTimeOffset = timeGetTime() + (72 - now);
    }
    self->serverTimeLagCount = 0;
}

void session_set_player_pos_dir(struct Session *self, int x, int y, int dir)
{
    self->playerDir = dir;
    self->playerPosX = x;
    self->playerPosY = y;
}
