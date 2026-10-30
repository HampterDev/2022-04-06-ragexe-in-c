#include "ui/chat/reported_players.h"
#include "ui/chat/reporter_controls.h"
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct Block { unsigned char *data; size_t bytes; } blocks[256];
static unsigned live, allocations;
static int expect_failure;
static jmp_buf failed;

void *mem_alloc(size_t bytes)
{
    unsigned i;
    unsigned char *raw;
    assert(bytes && bytes < 1000000);
    for (i = 0; i < 256 && blocks[i].data; ++i) {}
    assert(i < 256);
    raw = malloc(bytes + 32);
    assert(raw);
    memset(raw, 0xa7, 16);
    memset(raw + 16, 0x5b, bytes);
    memset(raw + 16 + bytes, 0x7a, 16);
    blocks[i].data = raw + 16;
    blocks[i].bytes = bytes;
    ++live;
    ++allocations;
    return blocks[i].data;
}

void mem_free(void *memory)
{
    unsigned i, j;
    unsigned char *data = memory;
    if (!memory) return;
    for (i = 0; i < 256 && blocks[i].data != data; ++i) {}
    assert(i < 256 && live);
    for (j = 0; j < 16; ++j) {
        assert(data[(int)j - 16] == 0xa7);
        assert(data[blocks[i].bytes + j] == 0x7a);
    }
    free(data - 16);
    blocks[i].data = NULL;
    --live;
}

__declspec(noreturn) void fatal_error(const char *message)
{
    assert(expect_failure && !strcmp(message, "vector too long"));
    longjmp(failed, 1);
}

static const char *names[] = {
    "봄", "a player name that requires allocated string storage", "", "two"
};

static void test_borrowed_name(void)
{
    struct String name;
    struct ReportedPlayer player;
    string_init(&name);
    string_assign_cstr(&name, names[1]);
    assert(reported_player_init(&player, &name, 19, UINT16_MAX) == &player);
    assert(!strcmp(string_cstr(&name), names[1]));
    assert(string_cstr(&name) != string_cstr(&player.name));
    string_free(&name);
    assert(!strcmp(string_cstr(&player.name), names[1]));
    assert(player.aid == 19 && player.job == UINT16_MAX);
    string_free(&player.name);
    assert(!live);
}

static void check_empty_record(const struct ReportedPlayer *player)
{
    struct ReportedPlayer empty;
    memset(&empty, 0, sizeof empty);
    string_init(&empty.name);
    assert(!memcmp(player, &empty, sizeof empty));
}

static void test_list(void)
{
    struct Vec players = {0};
    struct ReportedPlayer *p;
    size_t capacity, i, j;
    unsigned before;
    void *storage;

    vec_resize(&players, 0, sizeof(struct ReportedPlayer));
    reported_players_clear(&players);
    reported_players_free(&players);
    assert(!players.first && !players.last && !players.end && !live);

    for (i = 0; i < 80; ++i) {
        struct ReportedPlayer player;
        struct String name;
        const char *text = names[i % 4];
        string_init(&name);
        string_assign_cstr(&name, text);
        reported_player_init(&player, &name, (int)(i % 7), (uint16_t)(1000 + i));
        string_free(&name);
        vec_push(&players, &player, sizeof player);
        string_init(&player.name);
        assert(!player.name.size && !string_cstr(&player.name)[0]);
        string_free(&player.name);
        p = players.first;
        assert((struct ReportedPlayer *)players.last - p == (ptrdiff_t)i + 1);
        for (j = 0; j <= i; ++j) {
            assert(!strcmp(string_cstr(&p[j].name), names[j % 4]));
            assert(p[j].aid == (int)(j % 7) && p[j].job == 1000 + j);
        }
    }

    vec_resize(&players, 140, sizeof *p);
    p = players.first;
    for (i = 0; i < 80; ++i) {
        assert(!strcmp(string_cstr(&p[i].name), names[i % 4]));
        assert(p[i].aid == (int)(i % 7) && p[i].job == 1000 + i);
    }
    for (; i < 140; ++i) check_empty_record(&p[i]);
    storage = players.first;
    capacity = (struct ReportedPlayer *)players.end - p;
    for (i = 5; i < 140; ++i) string_free(&p[i].name);
    vec_resize(&players, 5, sizeof *p);
    assert(players.first == storage && live == 5);
    before = allocations;
    vec_resize(&players, capacity, sizeof *p);
    assert(players.first == storage && allocations == before);
    for (i = 5; i < capacity; ++i) check_empty_record(&p[i]);

    expect_failure = 1;
    if (!setjmp(failed)) {
        vec_resize(&players, SIZE_MAX / sizeof *p + 1, sizeof *p);
        assert(!"oversized list must fail");
    }
    expect_failure = 0;
    assert(players.first == storage && players.last == players.end && allocations == before);
    assert(!strcmp(string_cstr(&p[1].name), names[1]));

    reported_players_clear(&players);
    assert(players.first == storage && players.last == storage && live == 1);
    vec_resize(&players, 3, sizeof *p);
    for (i = 0; i < 3; ++i) check_empty_record(&p[i]);
    reported_players_clear(&players);
    assert(players.first == storage && players.last == storage);
    reported_players_free(&players);
    reported_players_free(&players);
    assert(!players.first && !players.last && !players.end && !live);
}

static void test_move_from_list(void)
{
    struct Vec players = {0};
    struct ReportedPlayer player, *p;
    struct String name;
    string_init(&name);
    string_assign_cstr(&name, names[1]);
    reported_player_init(&player, &name, 77, 501);
    string_free(&name);
    vec_push(&players, &player, sizeof player);
    assert(players.last == players.end);
    vec_push(&players, players.first, sizeof player);
    p = players.first;
    string_init(&p[0].name);
    assert(!p[0].name.size && !string_cstr(&p[0].name)[0]);
    assert(!strcmp(string_cstr(&p[1].name), names[1]));
    assert(p[0].aid == 77 && p[1].aid == 77 && p[1].job == 501);
    reported_players_free(&players);
    assert(!live);
}

static void test_borrowed_controls(void)
{
    struct Vec rows = {0}, windows = {0};
    struct ReporterRow row, *added, *list;
    struct UIWindow **queued;
    int controls[128];
    size_t i;

    for (i = 0; i < 128; ++i) controls[i] = (int)i + 100;
    for (i = 0; i < 32; ++i) {
        row.remove = (struct UIBitmapButton *)&controls[2 * i];
        row.name = (struct UIWindow *)&controls[2 * i + 1];
        added = vec_push(&rows, &row, sizeof row);
        assert(added == (struct ReporterRow *)rows.last - 1);
        assert(added->remove == row.remove && added->name == row.name);
    }
    list = rows.first;
    added = vec_insert(&rows, list + 3, list + 9, sizeof *list);
    list = rows.first;
    assert(added == list + 3);
    assert(added->remove == (struct UIBitmapButton *)&controls[18]);
    assert(added->name == (struct UIWindow *)&controls[19]);
    for (i = 0; i < 33; ++i) {
        size_t original = i < 3 ? i : i == 3 ? 9 : i - 1;
        assert(list[i].remove == (struct UIBitmapButton *)&controls[2 * original]);
        assert(list[i].name == (struct UIWindow *)&controls[2 * original + 1]);
    }
    for (i = 0; i < 100; ++i) {
        struct UIWindow *window = i % 5 ? (struct UIWindow *)&controls[i] : NULL;
        vec_push(&windows, &window, sizeof window);
    }
    queued = windows.first;
    assert(vec_insert(&windows, queued + 2, &queued[9], sizeof *queued) ==
           (struct UIWindow **)windows.first + 2);
    queued = windows.first;
    for (i = 0; i < 101; ++i) {
        size_t original = i < 2 ? i : i == 2 ? 9 : i - 1;
        assert(queued[i] == (original % 5 ? (struct UIWindow *)&controls[original] : NULL));
    }
    vec_free(&rows);
    vec_free(&windows);
    vec_free(&rows);
    vec_free(&windows);
    assert(!rows.first && !rows.last && !rows.end);
    assert(!windows.first && !windows.last && !windows.end && !live);
    for (i = 0; i < 128; ++i) assert(controls[i] == (int)i + 100);
}

static void test_zero_bytes(void)
{
    static const size_t counts[] = {0, 1, 6, 29, 256, 4097};
    size_t i, j;
    for (i = 0; i < sizeof counts / sizeof counts[0]; ++i) {
        struct Vec bytes;
        vec_init(&bytes);
        vec_resize(&bytes, counts[i], 1);
        assert(vec_count(&bytes, 1) == counts[i]);
        assert(vec_capacity(&bytes, 1) == counts[i]);
        for (j = 0; j < counts[i]; ++j) assert(!((unsigned char *)bytes.first)[j]);
        vec_free(&bytes);
        assert(!live);
    }
}

int main(void)
{
    test_borrowed_name();
    test_list();
    test_move_from_list();
    test_borrowed_controls();
    test_zero_bytes();
    puts("Reported players: borrowed names, Korean text, duplicate ids, moves, growth, zero initialization and guarded cleanup passed.");
    puts("Reporter controls: typed rows, borrowed windows, aliasing, middle insertion, growth, zero byte vectors and guarded cleanup passed.");
    return 0;
}
