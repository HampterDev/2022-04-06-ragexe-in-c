/* Real catalog lookup -> map entry / NPC packet -> sound manager -> handoff.
 * Hardware and owner storage are substituted; no window/device/server opens. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "std/string.h"
#include "std/vec.h"
#define MSS_IMPORT
#include "npc_bgm_sound.inc"

uint32_t g_soundMode = 1;
struct GameMode { const char *map_name; };
struct Session { struct Vec mp3Names; };
static struct Session g_session1;
#include "npc_bgm_lookup.inc"
static struct NamedText map_catalog[] = {
    {"empty.rsw", ""}, {"test.rsw", "bgm\\map.mp3"}
};
static int failures, starts, opens, closes, backend_status, backend_loops, fail_open;
static int driver_token, stream_token;
static const char *map_track = "bgm\\map.mp3";
static char current_track[128];
/* Measured with the shipped mss32.dll. Keep these independent of the client
 * header so a mistaken production constant cannot make this test pass. */
enum { BACKEND_DONE = 2, BACKEND_PLAYING = 4 };

#define CHECK(condition) do { if (!(condition)) { \
    printf("FAIL line %d: %s (track=%s starts=%d overridden=%d status=%d)\n", \
        __LINE__, #condition, current_track, starts, g_sound_settings.bgm_overridden, backend_status); \
    ++failures; } } while (0)

void *mem_alloc(size_t size) { void *p = malloc(size); assert(p); return p; }
void mem_free(void *p) { free(p); }
__declspec(noreturn) void fatal_error(const char *message)
{ fprintf(stderr, "%s\n", message); exit(2); }

void *__stdcall _AIL_open_stream(void *driver, const char *name, int memory)
{
    assert(driver == &driver_token && memory == 0);
    ++opens;
    if (fail_open) return NULL;
    strcpy_s(current_track, sizeof current_track, name);
    return &stream_token;
}
void __stdcall _AIL_close_stream(void *stream)
{ assert(stream == &stream_token); ++closes; backend_status = 0; }
void __stdcall _AIL_set_stream_volume(void *stream, int volume)
{ assert(stream == &stream_token && volume == 100); }
void __stdcall _AIL_set_stream_loop_count(void *stream, int loops)
{ assert(stream == &stream_token); backend_loops = loops; }
void __stdcall _AIL_start_stream(void *stream)
{ assert(stream == &stream_token); backend_status = BACKEND_PLAYING; ++starts; }
int __stdcall _AIL_stream_status(void *stream)
{ assert(stream == &stream_token); return backend_status; }
int __stdcall _AIL_3D_room_type(void *provider) { (void)provider; return -1; }
void __stdcall _AIL_set_3D_room_type(void *provider, int type)
{ (void)provider; (void)type; assert(0); }

static struct String *game_mode_get_map_name(struct GameMode *mode, struct String *name)
{ string_init(name); string_assign_cstr(name, mode->map_name); return name; }

#include "npc_bgm_functions.inc"

static struct SoundManager manager;
static void reset_case(void)
{
    if (sound_manager) {
        sound_stop_stream();
        free(manager.bgm_name);
        platform_mutex_destroy(manager.stream_lock);
    }
    memset(&manager, 0, sizeof manager);
    manager.stream_lock = platform_mutex_create();
    manager.driver = &driver_token;
    manager.bgm_volume = 100;
    sound_manager = &manager;
    g_sound_settings.bgm_on = 1;
    g_sound_settings.bgm_overridden = 0;
    starts = opens = closes = backend_status = fail_open = 0;
    current_track[0] = 0;
    g_session1.mp3Names = (struct Vec){map_catalog, map_catalog + 2, map_catalog + 2};
    sound_play_bgm(map_track);
}

static void request_music(struct GameMode *mode)
{
    static const unsigned char packet[] = {0xfe, 0x07, 'e', 'v', 'e', 'n', 't', 0};
    game_mode_zc_play_npc_bgm(mode, packet);
}

static void silent_map_cases(void)
{
    struct GameMode mode = {"missing.rsw"};
    int before;

    reset_case(); before = opens;
    CHECK(session_get_mp3_name(&g_session1, mode.map_name) == NULL);
    play_map_bgm(&mode);
    CHECK(!manager.stream && !manager.stream_name && !manager.bgm_name && closes == 1);
    sound_play_stream(NULL, 0);
    CHECK(opens == before && !sound_stream_playing());

    request_music(&mode);
    CHECK(sound_stream_playing() && backend_loops == 1);
    before = opens; backend_status = BACKEND_DONE;
    restart_bgm(&mode);
    CHECK(!manager.stream && !manager.bgm_name && !g_sound_settings.bgm_overridden);
    restart_bgm(&mode); sound_play_stream(NULL, 0);
    CHECK(opens == before);

    reset_case(); g_sound_settings.bgm_on = 0; sound_stop_stream();
    play_map_bgm(&mode); before = opens;
    g_sound_settings.bgm_on = 1; sound_play_stream(NULL, 0);
    CHECK(opens == before && !manager.bgm_name && !manager.stream);

    reset_case(); g_session1.mp3Names = (struct Vec){0}; before = opens;
    CHECK(session_get_mp3_name(&g_session1, "test.rsw") == NULL);
    play_map_bgm(&mode);
    CHECK(opens == before && !manager.bgm_name && !manager.stream);

    reset_case(); mode.map_name = "empty.rsw"; before = opens;
    play_map_bgm(&mode);
    CHECK(opens == before && !manager.bgm_name && !manager.stream);
    mode.map_name = "test.rsw"; play_map_bgm(&mode);
    CHECK(opens == before + 1 && sound_stream_playing() && backend_loops == 0);
    CHECK(strcmp(current_track, map_track) == 0);
}

int main(void)
{
    struct GameMode mode = {"test.rsw"};
    int before, i;

    reset_case();
    request_music(&mode);
    CHECK(strcmp(current_track, "bgm\\event.mp3") == 0 && backend_loops == 1);
    CHECK(g_sound_settings.bgm_overridden == 1);
    CHECK(sound_stream_playing());
    before = starts;
    for (i = 0; i < 3; ++i) restart_bgm(&mode);
    CHECK(starts == before && strcmp(current_track, "bgm\\event.mp3") == 0);
    CHECK(g_sound_settings.bgm_overridden == 1);
    backend_status = BACKEND_DONE;
    CHECK(!sound_stream_playing());
    restart_bgm(&mode);
    CHECK(starts == before + 1 && strcmp(current_track, map_track) == 0 && backend_loops == 0);
    CHECK(g_sound_settings.bgm_overridden == 0);
    restart_bgm(&mode);
    CHECK(starts == before + 1);

    /* Completion is also exercised without preceding playing-state updates. */
    reset_case(); request_music(&mode); before = starts;
    backend_status = BACKEND_DONE;
    restart_bgm(&mode);
    CHECK(starts == before + 1 && strcmp(current_track, map_track) == 0);
    CHECK(g_sound_settings.bgm_overridden == 0);

    reset_case(); before = starts;
    restart_bgm(&mode);
    backend_status = BACKEND_DONE; restart_bgm(&mode);
    CHECK(starts == before && g_sound_settings.bgm_overridden == 0);

    reset_case(); g_sound_settings.bgm_on = 0; sound_stop_stream();
    before = opens; request_music(&mode); restart_bgm(&mode);
    CHECK(opens == before && g_sound_settings.bgm_overridden == 0);

    reset_case(); request_music(&mode); g_sound_settings.bgm_on = 0;
    sound_stop_stream(); before = opens; restart_bgm(&mode);
    CHECK(opens == before); /* restoration must not override the user's mute */

    reset_case(); fail_open = 1; request_music(&mode); fail_open = 0;
    before = starts; restart_bgm(&mode);
    CHECK(starts == before + 1 && strcmp(current_track, map_track) == 0);
    CHECK(g_sound_settings.bgm_overridden == 0);

    silent_map_cases();

    sound_stop_stream(); free(manager.bgm_name);
    platform_mutex_destroy(manager.stream_lock); sound_manager = NULL;
    if (failures) { printf("NPC music handoff: %d failed checks\n", failures); return 1; }
    puts("Map/NPC music: mapped and silent entry, empty catalog/entry, resume, event completion, mute, missing track and return to mapped music passed");
    return 0;
}
