#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game/gamemode.h"
#include "game/mode_callbacks.h"
#include "world/mode_callbacks.h"
#include "world/mode_actions.h"
#include "ui/window_manager_state.h"
#include "ui/window_messages.h"
#include "game_mode_layout_unreached.h"

#ifdef GAME_MODE_LAYOUT_REFERENCE
#define GAME_BASE(mode) ((struct Mode *)(mode))
#define BASE_FIELD(base, current, previous) ((base)->previous)
#else
#define GAME_BASE(mode) (&(mode)->base)
#define BASE_FIELD(base, current, previous) ((base)->current)
#endif

struct UIWindowMgr g_windowMgr;
static struct GameMode *expected;
static void *blocks[4096], *mode_block;
static unsigned block_ids[4096], next_block_id;
static size_t block_count;
static int fail_mode_allocation, cursor_messages;
static uint32_t trace[65536];
static size_t trace_count;

static void record(uint32_t value)
{
    assert(trace_count < sizeof trace / sizeof *trace);
    trace[trace_count++] = value;
}

void *mem_alloc(size_t size)
{
    void *p = malloc(size);
    assert(p && block_count < sizeof blocks / sizeof *blocks);
    blocks[block_count] = p;
    block_ids[block_count++] = ++next_block_id;
    record(0xa110);
    record((uint32_t)size);
    record(next_block_id);
    return p;
}

void mem_free(void *p)
{
    size_t i;
    if (!p)
        return;
    for (i = 0; i < block_count; ++i) {
        if (blocks[i] == p) {
            record(0xfee);
            record(block_ids[i]);
            blocks[i] = blocks[--block_count];
            block_ids[i] = block_ids[block_count];
            free(p);
            return;
        }
    }
    assert(!"free of unowned mode storage");
}

__declspec(noreturn) void fatal_error(const char *message)
{
    fprintf(stderr, "%s\n", message);
    abort();
}

static void *mode_allocate(size_t size)
{
    assert(size == sizeof(struct GameMode) && !mode_block);
    record(0xc000);
    record(fail_mode_allocation);
    if (fail_mode_allocation)
        return NULL;
    mode_block = malloc(size);
    assert(mode_block);
    memset(mode_block, 0xa5, size);
    return mode_block;
}

static void mode_free(void *p)
{
    assert(p == mode_block && block_count == 0);
    record(0xd000);
    free(p);
    mode_block = NULL;
}

#define malloc mode_allocate
#define free mode_free
#include "../src/game/lifecycle.c"
#undef free
#undef malloc

int game_mode_on_run(struct Mode *base)
{
    assert(base == GAME_BASE(expected));
    record(1);
    return 73;
}

void game_mode_on_init(struct Mode *base, const char *name)
{
    assert(base == GAME_BASE(expected) && !strcmp(name, "prontera.rsw"));
    record(2);
}

void game_mode_on_exit(struct Mode *base)
{
    assert(base == GAME_BASE(expected));
    record(3);
}

void game_mode_on_update(struct Mode *base)
{
    assert(base == GAME_BASE(expected));
    ++expected->whispers_logged;
    record(4);
}

intptr_t game_mode_send_msg(struct Mode *base, int message, intptr_t a, intptr_t b, intptr_t c, intptr_t d)
{
    assert(base == GAME_BASE(expected) && message == 123 && a == -1 && b == 2 && c == 3 && d == 4);
    record(6);
    return 91;
}

void game_mode_on_change_state(struct Mode *base, int state)
{
    assert(base == GAME_BASE(expected));
    BASE_FIELD(base, next_sub_mode, nextSubMode) = state;
    record(7);
    record(state);
}

intptr_t ui_window_mgr_send_msg_(struct UIWindowMgr *manager, int message, intptr_t action, intptr_t b, intptr_t c, intptr_t d)
{
    assert(manager == &g_windowMgr && message == 0xf && !b && !c && !d);
    ++cursor_messages;
    record(8);
    record(action);
    return 0;
}

static void check_base(struct Mode *base)
{
    assert(base == (struct Mode *)expected);
    assert(BASE_FIELD(base, loop_cond, loopCond) == 1);
    assert(BASE_FIELD(base, next_sub_mode, nextSubMode) == -1);
    assert(BASE_FIELD(base, connected, connected) == 1);
    assert(BASE_FIELD(base, attack_in_progress, attack_in_progress) == 0);
    assert(BASE_FIELD(base, help_balloon, helpBalloon) == NULL);
    assert(BASE_FIELD(base, screenshot_pending, screenShotNow) == 0);
    assert(BASE_FIELD(base, cursor_offset_x, cursorOffsetX) == 0);
    assert(BASE_FIELD(base, cursor_offset_y, cursorOffsetY) == 0);
    assert(BASE_FIELD(base, cursor_action, cursorAction) == 0);
    assert(BASE_FIELD(base, cursor_motion, cursorMotion) == 0);
    assert(BASE_FIELD(base, fade_in_start, fadeInStart) == 0);
}

static void populate_strings(struct GameMode *mode, int round)
{
    struct String name;
    struct String *strings[] = {&mode->whisper_setting_name,
                                   &mode->exchange_partner,
                                   &mode->popup_target_name,
                                   &mode->whisper_text,
                                   &mode->whisper_target,
                                   &mode->drag.display_name,
                                   &mode->drag.name,
                                   &mode->drag.name2,
                                   &mode->own_chat_room.title,
                                   &mode->own_chat_room.password,
                                   &mode->illust_file,
                                   &mode->last_chat_line};
    const char *text = round & 1 ? "긴 스크린샷 이름과 게임 상태 문자열" : "이름";
    size_t i;
    for (i = 0; i < sizeof strings / sizeof *strings; ++i)
        string_assign_cstr(strings[i], text);
    for (i = 0; i < 11; ++i)
        string_assign_cstr(&mode->chat_history[i], text);
    string_init(&name);
    string_assign_cstr(&name, text);
    name_card_holder_set_name(&mode->actor_names, 42, name);
    record((uint32_t)strlen(text));
}

int main(int argc, char **argv)
{
    int round;
    size_t i;
    uint32_t digest = UINT32_C(2166136261);
    fail_mode_allocation = 1;
    assert(!game_mode_create() && !block_count && !mode_block);
    fail_mode_allocation = 0;
    for (round = 0; round < 32; ++round) {
        struct Mode *base;
        expected = game_mode_create();
        assert(expected && mode_block == expected);
        base = GAME_BASE(expected);
        check_base(base);
        assert(base->ops == &g_gameModeVtable_ops);
        assert(!expected->world && !expected->view && !expected->mouse_pointer);
        assert(!expected->skill_use.type && !expected->skill_use_unchecked);
        assert(!expected->actor_names.cards.size && expected->pickup_requests.head);
        populate_strings(expected, round);
        expected->whispers_logged = 0;
        game_mode_on_init(base, "prontera.rsw");
        assert(game_mode_on_run(base) == 73);
        mode_update(base);
        assert(expected->whispers_logged == 1);
        assert(mode_send_message(base, 123, -1, 2, 3, 4) == 91);
        game_mode_on_change_state(base, round);
        assert(BASE_FIELD(base, next_sub_mode, nextSubMode) == round);
        cursor_messages = 0;
        mode_set_cursor_action(base, 3);
        BASE_FIELD(base, cursor_motion, cursorMotion) = 9;
        mode_set_cursor_action(base, 3);
        assert(cursor_messages == 1 && BASE_FIELD(base, cursor_motion, cursorMotion) == 9);
        mode_set_cursor_action(base, 7);
        assert(cursor_messages == 2 && BASE_FIELD(base, cursor_motion, cursorMotion) == 0);
        game_mode_on_exit(base);
        if (round & 1) {
            game_mode_destruct(expected);
            assert(!block_count);
            mode_free(expected);
        } else {
            game_mode_destroy(base);
        }
        assert(!block_count && !mode_block);
    }
    for (i = 0; i < trace_count * sizeof *trace; ++i)
        digest = (digest ^ ((const unsigned char *)trace)[i]) * UINT32_C(16777619);
    /* Removing the screenshot prefix eliminates one allocation/free per round. */
    assert(trace_count == 6562 && digest == UINT32_C(0x274efe64));
    if (argc == 2) {
        FILE *file = fopen(argv[1], "wb");
        assert(file && fwrite(trace, sizeof *trace, trace_count, file) == trace_count);
        assert(!fclose(file));
    }
    printf("Game mode layout: base state, callbacks, cursor, strings and cleanup passed; %zu trace "
           "words; size=%zu; digest=%08x.\n",
           trace_count, sizeof(struct GameMode), digest);
    return 0;
}
