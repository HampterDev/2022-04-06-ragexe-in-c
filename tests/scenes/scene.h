#ifndef TESTS_SCENES_SCENE_H
#define TESTS_SCENES_SCENE_H

#include <stddef.h>
#include <stdint.h>

/* The scene harness is the whole client with a few files swapped: the clock (scene_clock.c), the
   sockets (scene_net.c, an in-process map server), the threads (scene_thread.c), the settings
   store (scene_settings.c), the per-frame driver hook (scene_driver.c, which runs a Lua scene
   script), on Windows and macOS the window (scene_app_hidden.c, never shown) and on Windows the
   message boxes (scene_window_win.c). The driver's map server keeps the staged actors in
   scene_world.c. */

#define SCENE_MAP_PORT 5121
#define SCENE_FRAME_MS 16

void scene_clock_advance(uint32_t ms);

/* Server side of the fake sockets. */
typedef void (*SceneServerPacket)(int port, const unsigned char *packet, int length);
void scene_net_set_handler(SceneServerPacket handler);
int scene_net_deliver(int port, const void *data, int size);
int scene_net_connected(int port);

/* The fake map server's units (scene_world.c): where the actors a scene stages are and what they
   do, told to the client as the project's server tells it. Each returns 0 when a packet could
   not be delivered or the unit has nothing to act with. */
/* The next map loaded puts the player `aid` on (x, y), facing dir, at speed ms a cell (0: as
   before). */
void scene_world_place(uint32_t aid, int x, int y, int dir, int speed);
/* The client says the map has loaded (CZ_NOTIFY_ACTORINIT): the player is placed on it. */
int scene_world_loaded(void);
/* A timed exchange the client began at its tick `client_tick` (CZ_REQUEST_TIME2) is answered. */
int scene_world_time(uint32_t client_tick);
/* Once a frame: the units walk on to now. */
void scene_world_frame(void);
/* Stamps the coming of `aid`, directly before its entry, and puts its body on (x, y). */
int scene_world_spawn(uint32_t aid, int player, int x, int y, int dir, int speed);
/* Stamps the going of `aid`, directly before its vanish, unless `how` says a player died. */
int scene_world_vanish(uint32_t aid, int how);
/* `aid` walks to (x, y) by the server's path; 0 also when there is no way there. */
int scene_world_walk(uint32_t aid, int x, int y);

struct SceneAttack {
    uint32_t source, target;
    int damage, left_damage, count, kind, sp_damage;
    unsigned motion_ms, flinch_ms;
};
/* A swing of `source` at `target` begins now and lands motion_ms later with its hit. */
int scene_world_attack(const struct SceneAttack *attack);

struct SceneCast {
    uint32_t source, target;
    int x, y, element, disposable;
    unsigned skill_id, skill_lv, cast_ms, motion_ms;
};
/* A cast of `source` begins now and ends cast_ms later. */
int scene_world_cast(const struct SceneCast *cast);

#endif
