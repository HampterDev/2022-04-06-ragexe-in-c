#include "game/packet_handlers.h"
#include "ui/window_access.h"
#include "ui/chat/balloon.h"
#include <string.h>
#include <time.h>
#include "game/gamemode.h"
#include "game/packets_inventory.h"
#include "platform/language.h"
#include "core/msg_str.h"
#include "ui/window.h"
#include "platform/clock.h"

enum {
    SERVICE_TYPE_CHINA = 3,
    SERVICE_TYPE_GERMANY = 10,
    SERVICE_TYPE_BRAZIL = 12,
};

#pragma pack(push, 1)
struct PACKET_ZC_PCBANG_EFFECT {
    unsigned short id;
    int exp_factor;
    int death_factor;
    int drop_factor;
};
struct PACKET_ZC_REPLY_REMAINTIME {
    unsigned short id;
    int result;
    __time32_t expiration_date;
    int remain_time;
};
#pragma pack(pop)

#define CAFE_NOTICE_COLOR 0x62b0ff

void game_mode_zc_pc_bang_effect(struct GameMode *self, const struct PACKET_ZC_PCBANG_EFFECT *packet)
{
    char text[200];
    struct UIWindow *wnd;
    struct UIBalloonText *balloon;
    int x;

    self->notice_shown_at = platform_ticks_ms();
    if (self->notice_wnd == 0) {
        wnd = mem_alloc(sizeof(struct UITransBalloonText));
        if (wnd != 0)
            wnd = ui_trans_balloon_text_construct(wnd);
        self->notice_wnd = wnd;
        ui_window_create(wnd, 8, 8);
        ui_window_mgr_add_to_window_list(&g_windowMgr, self->notice_wnd);
    }
    sprintf(text, msg_str(0x4f7), packet->exp_factor, packet->death_factor, packet->drop_factor);
    if (g_serviceType == SERVICE_TYPE_BRAZIL)
        sprintf(text, msg_str(0x465));
    if (strlen(text) != 0) {
        balloon = (struct UIBalloonText *)self->notice_wnd;
        ui_balloon_text_set_text(balloon, text, 0x48);
        x = (g_renderer->width - 640) / 2 - self->notice_wnd->width / 2 + 320;
        ui_window_move_safe(self->notice_wnd, x, 0x33);
        balloon->colour = CAFE_NOTICE_COLOR;
        balloon->outline = 0;
    }
    if (g_serviceType == 0)
        ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (int)text, CAFE_NOTICE_COLOR, 0, 0);
}

enum {
    REMAIN_DAYS = 1,
    REMAIN_HOURS = 2,
    REMAIN_PCBANG_FIRST = 0x8000,
    REMAIN_PCBANG_LAST = 0x80ff,
    REMAIN_MIXED = 100,
    REMAIN_PCBANG_REFUSED = 1000,
    REMAIN_FREE_2H = 10000,
};
enum {
    PLAN_PREMIUM = 0x01,
    PLAN_EXP_UP = 0x02,
    PLAN_ITEM_UP = 0x04,
    PLAN_DEATH_DOWN = 0x08,
    PLAN_ADVANCED_PACKAGE = 0x10,
    PLAN_CAFE_SILVER = 0x20,
    PLAN_CAFE_GOLD = 0x40,
    PLAN_CAFE_SPECIAL = 0x80,
};

void game_mode_zc_reply_remain_time(struct GameMode *self, const struct PACKET_ZC_REPLY_REMAINTIME *packet)
{
    char text[0x200];
    char tail[0x100];
    __time32_t now;
    struct tm *end;
    int result, remain;

    (void)self;
    memset(text, 0, sizeof text);
    if (g_serviceType == SERVICE_TYPE_CHINA) {
        if (packet->result == REMAIN_DAYS) {
            sprintf(text, msg_str(0x464), packet->expiration_date / 86400 + 1);
        } else if (packet->result == REMAIN_HOURS) {
            remain = packet->remain_time;
            sprintf(text, msg_str(0x465), remain / 3600, remain % 3600 / 60, remain % 60);
        }
    } else {
        _time32(&now);
        _localtime32(&now);
        end = _localtime32(&packet->expiration_date);
        result = packet->result;
        if (result >= REMAIN_PCBANG_FIRST && result <= REMAIN_PCBANG_LAST) {
            if (result & PLAN_CAFE_SILVER)
                strcat(text, "(PC방-실버) ");
            if (result & PLAN_CAFE_GOLD)
                strcat(text, "(PC방-골드) ");
            if (result & PLAN_CAFE_SPECIAL)
                strcat(text, "(PC방-스페셜) ");
            if (result & PLAN_PREMIUM) {
                strcat(text, "프리미엄 정액 요금제");
                if (result & PLAN_ADVANCED_PACKAGE) {
                    strcat(text, " + Advanced Package");
                } else {
                    if (result & PLAN_EXP_UP)
                        strcat(text, " + Exp UP");
                    if (result & PLAN_ITEM_UP)
                        strcat(text, " + Item UP");
                    if (result & PLAN_DEATH_DOWN)
                        strcat(text, " + Death Down");
                    memset(tail, 0, sizeof tail);
                    remain = packet->remain_time;
                    if (packet->expiration_date > now) {
                        if (remain > 0)
                            sprintf(tail, " (종료일시 : %04d/%02d/%02d , "
                                        "남은시간 : %d 시간 %d 분 %d 초)",
                                        end->tm_year + 1900, end->tm_mon + 1, end->tm_mday,
                                        remain / 3600, remain / 60 % 60, remain % 60);
                        else if (remain == 0)
                            sprintf(tail, " (종료일시 : %04d/%02d/%02d )",
                                        end->tm_year + 1900, end->tm_mon + 1, end->tm_mday);
                    } else if (remain > 0) {
                        sprintf(tail, " (남은시간 : %d 시간 %d 분 %d 초)",
                                    remain / 3600, remain / 60 % 60, remain % 60);
                    }
                    strcat(text, tail);
                }
            }
        } else if (result == REMAIN_MIXED) {
            remain = packet->remain_time;
            if (packet->expiration_date > now) {
                if (remain > 0)
                    sprintf(text, "귀하는 혼합요금제 "
                                "사용자 입니다. (종료일시 : "
                                "%04d/%02d/%02d , 남은시간 : %d 시간 %d 분 %d "
                                "초)",
                                end->tm_year + 1900, end->tm_mon + 1, end->tm_mday,
                                remain / 3600, remain / 60 % 60, remain % 60);
                else if (remain == 0)
                    sprintf(text, "귀하는 정액요금제 "
                                "사용자 입니다. (종료일시 : "
                                "%04d 년 %02d 월 %02d 일 %02d 시 %02d 분 %02d 초)",
                                end->tm_year + 1900, end->tm_mon + 1, end->tm_mday,
                                end->tm_hour, end->tm_min, end->tm_sec);
            } else if (remain > 0) {
                sprintf(text, "귀하는 정량요금제 "
                            "사용자 입니다. (남은시간 : "
                            "%d 시간 %d 분 %d 초)",
                            remain / 3600, remain / 60 % 60, remain % 60);
            }
        } else if (result == REMAIN_PCBANG_REFUSED) {
            sprintf(text, "PC방의 요금제 정보는 "
                        "요청할 수 없습니다.");
        } else if (result == REMAIN_FREE_2H) {
            sprintf(text, "귀하는 사크라이 2시간 "
                        "무료요금제 사용자 입니다.");
        } else {
            sprintf(text, "과금정보 요청에 실패 "
                        "했습니다.");
        }
    }
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (int)text, 0xffffff, 0, 0);
}

#pragma pack(push, 1)
struct PACKET_ZC_INFO_REMAINTIME {
    unsigned short id;
    int type;
    int remain_time;
};
#pragma pack(pop)

enum {
    SERVICE_TYPE_INDONESIA = 6,
    REMAIN_PERSONAL_FLAT = 100,
    REMAIN_PERSONAL_METERED = 200,
    REMAIN_CAFE_METERED = 300,
};
#define REMAIN_TIME_COLOR 0xffffff

void game_mode_zc_info_remain_time(struct GameMode *self, const struct PACKET_ZC_INFO_REMAINTIME *packet)
{
    char text[0x100];
    int remain = packet->remain_time;

    (void)self;
    if (g_serviceType == 0) {
        switch (packet->type) {
        case REMAIN_PERSONAL_FLAT:
            sprintf(text, "개인 정액제 시간이 %d분 %d초 "
                        "남았습니다.", remain / 60, remain % 60);
            break;
        case REMAIN_PERSONAL_METERED:
            sprintf(text, "개인 정량제 시간이 %d분 %d초 "
                        "남았습니다.", remain / 60, remain % 60);
            break;
        case REMAIN_CAFE_METERED:
            sprintf(text, "PC방 정량제 시간이 %d분 %d초 "
                        "남았습니다.", remain / 60, remain % 60);
            break;
        }
    } else if (g_serviceType == SERVICE_TYPE_INDONESIA) {
        sprintf(text, "Sisa waktu voucher kamu adalah %d menit.", remain / 60);
    } else {
        return;
    }
    ui_window_mgr_send_msg_(&g_windowMgr, UIM_PUSH_INTO_CHAT, (int)text, REMAIN_TIME_COLOR, 0, 0);
}
