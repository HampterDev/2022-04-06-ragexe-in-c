#include "session/state.h"
#include "ui/window_manager_state.h"
#include "login/loginmode.h"

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <winsock2.h>
#include "session/session_names.h"
#include "core/memory.h"

struct CharInfo g_loginCharInfo;

#define SUBMODE_SELECT_CHAR     7

#pragma pack(push, 1)
struct PACKET_HC_ACCEPT_ENTER_NEO_UNION_HEADER {
    WORD packet_type;
    WORD packet_length;
    BYTE normal_slots;
    BYTE premium_slots;
    BYTE billing_slots;
    BYTE producible_slots;
    BYTE valid_slots;
    struct LoginBillingInfo billing;
};
struct PACKET_HC_ACCEPT_ENTER {
    WORD packet_type;
    WORD packet_length;
    struct LoginBillingInfo billing;
    BYTE zero[3];
    struct CharInfo chars[];
};
struct PACKET_HC_ACCEPT_ENTER_NEO_UNION_LIST {
    WORD packet_type;
    short packet_length;
    struct CharInfo chars[];
};
struct PACKET_HC_CHAR_LIST {
    WORD packet_type;
    WORD packet_length;
    struct CharInfo chars[];
};
#pragma pack(pop)
SIZE_ASSERT(PACKET_HC_ACCEPT_ENTER_NEO_UNION_HEADER, 0x1d);
SIZE_ASSERT(PACKET_HC_ACCEPT_ENTER, 0x1b);
SIZE_ASSERT(PACKET_HC_ACCEPT_ENTER_NEO_UNION_LIST, 4);
SIZE_ASSERT(PACKET_HC_CHAR_LIST, 4);

static void list_error_box(const char *text)
{
    ui_window_mgr_message_box(&g_windowMgr, text, 0, 1, 1, 0, "메시지", 0x118, 0x78, 0);
}

static void billing_swap(struct LoginMode *self)
{
    self->billing.value1 = ntohl(self->billing.value1);
    self->billing.value5 = ntohl(self->billing.value5);
    self->billing.value9 = ntohl(self->billing.value9);
}

static void map_display_name(char *mapName)
{
    snprintf(mapName, 16, "%s", session_get_rsw_name(mapName));
}

int login_mode_max_char_slot(struct LoginMode *self)
{
    int max = -1;
    int i;

    if (!self)
        return 0;
    for (i = 0; i < LOGIN_MAX_CHARS; i++) {
        int slot;
        if (self->chars[i].gid == 0)
            break;
        slot = self->chars[i].slot;
        if (max < slot)
            max = slot;
    }
    return max;
}

void login_mode_remove_deleted_char(struct LoginMode *self)
{
    unsigned n = (unsigned)self->numChar;
    struct CharInfo *kept;
    int i, j;

    kept = mem_alloc(n > 0xffffffffu / sizeof(struct CharInfo) ? 0xffffffffu : n * sizeof(struct CharInfo));
    for (i = 0, j = 0; i < self->numChar; i++) {
        if (self->chars[i].slot == g_loginCharInfo.slot)
            continue;
        memcpy(&kept[j], &self->chars[i], sizeof(struct CharInfo));
        j++;
    }
    self->numChar--;
    g_session1.charListCounts[0]--;
    mem_copy_checked(self->chars, sizeof self->chars, kept, self->numChar * sizeof(struct CharInfo));
    mem_free(kept);
    self->base.next_sub_mode = SUBMODE_SELECT_CHAR;
}

void login_mode_on_char_list_header(struct LoginMode *self, const BYTE *pkt)
{
    const struct PACKET_HC_ACCEPT_ENTER_NEO_UNION_HEADER *packet =
        (const struct PACKET_HC_ACCEPT_ENTER_NEO_UNION_HEADER *)pkt;

    g_session1.charListCounts[1] = packet->normal_slots;
    g_session1.charListCounts[2] = packet->premium_slots;
    g_session1.charListCounts[3] = packet->billing_slots;
    g_session1.charListCounts[4] = packet->valid_slots;
    g_session1.charListCounts[6] = packet->producible_slots;
    memset(self->chars, 0, sizeof self->chars);
    g_session1.charListCounts[0] = 0;
    self->chars_received = 0;
    self->char_list_pages = 0;
    memcpy(&self->billing, &packet->billing, sizeof self->billing);
    billing_swap(self);
}

void login_mode_on_accept_enter(struct LoginMode *self, const BYTE *pkt)
{
    const struct PACKET_HC_ACCEPT_ENTER *packet = (const struct PACKET_HC_ACCEPT_ENTER *)pkt;
    char text[256];
    unsigned body;
    int rest, i;

    memcpy(&self->billing, &packet->billing, sizeof self->billing);
    billing_swap(self);
    body = (unsigned)packet->packet_length - 0x1b;
    self->numChar = body / sizeof(struct CharInfo);
    rest = body % sizeof(struct CharInfo);
    mem_copy_checked(self->chars, sizeof self->chars, packet->chars, self->numChar * sizeof(struct CharInfo));
    if (rest > 0) {
        memset(text, 0, sizeof text);
        snprintf(text, sizeof text,
                 "CHARACTER_INFO2 size error!! character num : %d  CHARACTER_INFO2 size : %d",
                 self->numChar, (int)sizeof(struct CharInfo));
        list_error_box(text);
    }
    g_session1.charListCounts[0] = self->numChar;
    for (i = 0; i < self->numChar; i++) {
        char *map = self->chars[i].map_name;
        size_t len = strlen(map);
        strncpy(map, session_get_rsw_name(map), len);
    }
}

void login_mode_on_accept_make_char(struct LoginMode *self, const struct CharInfo *chara)
{
    memcpy(&g_loginCharInfo, chara, sizeof g_loginCharInfo);
    memcpy(&self->chars[self->numChar], chara, sizeof(struct CharInfo));
    map_display_name(self->chars[self->numChar].map_name);
    self->numChar++;
    g_session1.charListCounts[0]++;
    self->base.next_sub_mode = SUBMODE_SELECT_CHAR;
}

void login_mode_on_char_list_page(struct LoginMode *self, const BYTE *pkt)
{
    const struct PACKET_HC_ACCEPT_ENTER_NEO_UNION_LIST *packet =
        (const struct PACKET_HC_ACCEPT_ENTER_NEO_UNION_LIST *)pkt;
    char text[256];
    __time32_t now;
    unsigned body = (unsigned)(packet->packet_length - 4);
    int count = body / sizeof(struct CharInfo);
    int base, i;

    if (body % sizeof(struct CharInfo)) {
        memset(text, 0, sizeof text);
        snprintf(text, sizeof text, "CHARACTER_INFO2 size error!! CHARACTER_INFO2 size : %d",
                 (int)sizeof(struct CharInfo));
        list_error_box(text);
        return;
    }
    base = self->chars_received;
    if ((unsigned)(base + count) > LOGIN_MAX_CHARS) {
        memset(text, 0, sizeof text);
        snprintf(text, sizeof text, "CHARACTER_INFO2 Cnt Over %d / %d", base + count, LOGIN_MAX_CHARS);
        list_error_box(text);
        return;
    }
    mem_copy_checked(&self->chars[base], sizeof self->chars - base * sizeof(struct CharInfo),
                       packet->chars, count * sizeof(struct CharInfo));
    _time32(&now);
    for (i = 0; i < count; i++) {
        struct CharInfo *c = &self->chars[self->chars_received + i];
        if (c->sex == 0x63)
            c->sex = (BYTE)g_session1.accountSex;
        if (c->delete_date)
            c->delete_date = now + c->delete_date;
        if ((WORD)(c->job - 4001) <= 6 && (short)c->body_palette < 1)
            c->body_palette = 1;
    }
    self->chars_received += count;
    if (count == 3)
        return;

    self->numChar = self->chars_received;
    g_session1.charListCounts[0] = self->chars_received;
    self->chars_received = 0;
    self->char_deleted = 0;
    for (i = 0; i < self->numChar; i++)
        map_display_name(self->chars[i].map_name);
    self->base.next_sub_mode = SUBMODE_SELECT_CHAR;
}
void login_mode_on_char_list(struct LoginMode *self, const BYTE *pkt)
{
    const struct PACKET_HC_CHAR_LIST *packet = (const struct PACKET_HC_CHAR_LIST *)pkt;

    self->numChar = ((unsigned)packet->packet_length - 4) / sizeof(struct CharInfo);
    g_session1.charListCounts[0] = self->numChar;
    mem_copy_checked(self->chars, sizeof self->chars, packet->chars, self->numChar * sizeof(struct CharInfo));
    self->base.next_sub_mode = SUBMODE_SELECT_CHAR;
}
