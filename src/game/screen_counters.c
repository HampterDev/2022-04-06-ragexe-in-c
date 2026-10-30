#include "game/packet_handlers.h"
#include "session/state.h"
#include "contents/mouse.h"
#include <windows.h>
#include <mmsystem.h>
#include "game/screen_counters.h"

#define GLYPH_SEPARATOR 10
#define DRAW_COLOR_WHITE 0xffffffffu
#define DRAW_COLOR_SKILL_LEVEL 0xfaffffffu
#define DRAW_FLAGS_SCREEN 0x201

int g_timeAttackBannerFrame;

static struct SkillLevelDigits {
    int level;
    int count;
    int digits[4];
} g_skillLevelDigits = {-1, 1, {9, 9, 9, 9}};

static void draw_glyph(int x, int y, struct ActRes *act, struct SprRes *spr,
                       int action, int motion, float scale, unsigned color)
{
    screen_sprite_frame(g_renderer, x, y, act, spr, action, motion, scale, 0, color, DRAW_FLAGS_SCREEN);
}

static int digit_count(int n)
{
    if (n > 999)
        return 4;
    if (n > 99)
        return 3;
    return n > 9 ? 2 : 1;
}

void game_mode_draw_pk_ranking(struct GameMode *self)
{
    const struct Player *player = self->world->player;
    int participants = player->pvp_total;
    int rank = player->pvp_rank;
    struct ActRes *act;
    struct SprRes *spr;
    int digits[4], count, n, i, x, y;

    if (participants <= 0 || rank <= 0)
        return;
    act = res_get("이팩트\\RankFont.act");
    spr = res_get("이팩트\\RankFont.spr");

    n = participants > 9999 ? 9999 : participants;
    if (n < 0)
        n = 0;
    count = digit_count(n);
    digits[0] = n % 10;
    digits[1] = n / 10 % 10;
    digits[2] = n / 100 % 10;
    digits[3] = n / 1000;
    y = g_renderer->height - 30;
    for (i = 0; i < count; i++)
        draw_glyph(g_renderer->width - 80 + 25 * (count - i), y, act, spr, digits[i], 0, 1.0f, DRAW_COLOR_WHITE);

    draw_glyph(g_renderer->width - 80, g_renderer->height - 40, act, spr, GLYPH_SEPARATOR, 0, 1.0f,
               DRAW_COLOR_WHITE);

    count = digit_count(rank);
    digits[0] = rank % 10;
    digits[1] = rank / 10 % 10;
    digits[2] = rank / 100 % 10;
    digits[3] = rank / 1000;
    y = g_renderer->height - 50;
    x = g_renderer->width - 105;
    for (i = 0; i < count; i++, x -= 25)
        draw_glyph(x, y, act, spr, digits[i], 0, 1.0f, DRAW_COLOR_WHITE);
}

static void draw_time_attack_banner(void)
{
    struct ActRes *act = res_get("이팩트\\TimeAttack.act");
    struct SprRes *spr = res_get("이팩트\\TimeAttack.spr");
    int frame = g_timeAttackBannerFrame / 3;
    const struct SprImg *image = sprite_resource_image(spr, 0, 0);

    if (!image || frame >= (int)actres_frame_count(act, 0))
        return;
    draw_glyph(g_renderer->width / 2 - image->height / 2, 120, act, spr, 0, frame, 1.0f,
               DRAW_COLOR_WHITE);
    g_timeAttackBannerFrame++;
}

void game_mode_draw_ranking_time_attack(struct GameMode *self)
{
    const struct Player *player = self->world->player;

    if (player->pvp_total <= 0 || player->pvp_rank <= 0)
        return;
    if (player->pvp_total <= 1 || player->pvp_rank != 1)
        return;
    game_mode_draw_time_attack(self, g_session1.pvpFirstPlaceTime, TIMER_SINCE);
    if (g_session1.pvpFirstPlaceTime == 0 || timeGetTime() - g_session1.pvpFirstPlaceTime >= 3000)
        return;
    draw_time_attack_banner();
}

void game_mode_draw_time_attack(struct GameMode *self, DWORD value, int type)
{
    struct ActRes *act;
    struct SprRes *spr;
    unsigned shown;
    int seconds, minutes = 0, hours = 0, days = 0;
    int glyphs[14] = { 0 };
    int field_count, glyph_count, i, x, half_width;

    if (type == TIMER_FOR_5S) {
        shown = value;
        if (self->timer_shown_since == 0)
            self->timer_shown_since = timeGetTime();
        if (timeGetTime() - self->timer_shown_since > 5000) {
            self->timer_shown_since = 0;
            g_session1.timerShown = 0;
        }
    } else if (type == TIMER_SINCE) {
        shown = timeGetTime() - value;
        if (value == 0)
            g_session1.timerShown = 0;
    } else if (type == TIMER_UNTIL) {
        shown = value - timeGetTime();
        if (shown == 0)
            g_session1.timerShown = 0;
    } else if (type == TIMER_COUNTDOWN) {
        unsigned ticks = g_session1.countdownTicks;
        shown = ticks / 30;
        g_session1.countdownTicks = ticks - 1;
        if (shown == 0)
            g_session1.timerShown = 0;
    } else {
        self->timer_shown_since = 0;
        g_session1.timerShown = 0;
        return;
    }

    seconds = (int)(shown / 1000);
    if (seconds >= 60) {
        minutes = seconds / 60;
        seconds -= minutes * 60;
        if (minutes >= 60) {
            hours = minutes / 60;
            minutes -= hours * 60;
            if (hours >= 24) {
                days = hours / 24;
                hours -= days * 24;
            }
        }
    }

    act = res_get("이팩트\\TimeFont.act");
    spr = res_get("이팩트\\TimeFont.spr");
    if (type != TIMER_COUNTDOWN) {
        shown = (unsigned)seconds;
    } else {
        days = hours = minutes = 0;
    }

    {
        int n = (int)shown > 9999 ? 9999 : (int)shown;
        if (n < 0)
            n = 0;
        glyphs[0] = n % 10;
        glyphs[1] = n / 10 % 10;
    }
    field_count = 2;
    if (minutes > 0) {
        field_count = 4;
        glyphs[3] = minutes % 10;
        glyphs[4] = minutes / 10 % 10;
    }
    if (hours > 0) {
        field_count = 6;
        glyphs[6] = hours % 10;
        glyphs[7] = hours / 10 % 10;
    }
    if (days > 0) {
        field_count = 8;
        glyphs[9] = days % 10;
        glyphs[10] = days / 10 % 10;
    }

    half_width = g_renderer->width / 2;
    glyph_count = field_count + (field_count - 2) / 2;
    x = half_width - 20 * glyph_count;
    for (i = 1; i <= glyph_count; i++, x += 40) {
        int frame = i % 3 == 0 ? GLYPH_SEPARATOR : glyphs[glyph_count - i];
        draw_glyph(x, 40, act, spr, frame, 0, 1.0f, DRAW_COLOR_WHITE);
    }
}

void game_mode_draw_skill_level_at_cursor(struct GameMode *self)
{
    struct ActRes *act;
    struct SprRes *spr;
    int level, i;

    if (self->skill_use.type == 0 || self->skill_use.level == 0)
        return;
    level = self->skill_use.level;
    if (level != g_skillLevelDigits.level) {
        int n;
        g_skillLevelDigits.level = level;
        n = level > 9999 ? 9999 : level;
        if (n < 0)
            n = 0;
        g_skillLevelDigits.count = digit_count(n);
        g_skillLevelDigits.digits[0] = n % 10;
        g_skillLevelDigits.digits[1] = n / 10 % 10;
        g_skillLevelDigits.digits[2] = n / 100 % 10;
        g_skillLevelDigits.digits[3] = n / 1000;
    }
    act = res_get("이팩트\\숫자.act");
    spr = res_get("이팩트\\숫자.spr");
    for (i = 0; i < g_skillLevelDigits.count; i++) {
        float x = (float)g_mouse.x + self->base.cursor_offset_x + 24.0f - (float)(16 * i)
                + (float)(16 * g_skillLevelDigits.count - 16);
        float y = (float)g_mouse.y + self->base.cursor_offset_y;
        draw_glyph((int)x, (int)y, act, spr, 0, g_skillLevelDigits.digits[i], 2.0f,
                   DRAW_COLOR_SKILL_LEVEL);
    }
}

#pragma pack(push, 1)
struct PACKET_ZC_SHOWDIGIT {
    unsigned short id;
    unsigned char type;
    int value;
};
#pragma pack(pop)

void game_mode_zc_show_digit(struct GameMode *self, const struct PACKET_ZC_SHOWDIGIT *packet)
{
    (void)self;
    switch (packet->type) {
    case TIMER_FOR_5S:
        g_session1.timerKind = TIMER_FOR_5S;
        g_session1.countdownTicks = (unsigned)packet->value * 1000u;
        break;
    case TIMER_SINCE:
        g_session1.timerKind = TIMER_SINCE;
        g_session1.countdownTicks = timeGetTime() + (unsigned)packet->value * 1000u;
        break;
    case TIMER_UNTIL:
        g_session1.timerKind = TIMER_UNTIL;
        g_session1.countdownTicks = timeGetTime() - (unsigned)packet->value * 1000u;
        break;
    case TIMER_COUNTDOWN:
        g_session1.timerKind = TIMER_COUNTDOWN;
        g_session1.countdownTicks = (unsigned)packet->value * 30u + 29u;
        break;
    }
    g_session1.timerShown = 1;
}
