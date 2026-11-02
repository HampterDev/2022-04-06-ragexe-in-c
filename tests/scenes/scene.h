#ifndef TESTS_SCENES_SCENE_H
#define TESTS_SCENES_SCENE_H

#include <stddef.h>
#include <stdint.h>

/* The scene harness is the whole client with a few files swapped: the clock (scene_clock.c), the
   sockets (scene_net.c, an in-process map server), the threads (scene_thread.c), the settings
   store (scene_settings.c), on Windows the cursor and key state (scene_input_win.c), and the
   per-frame driver hook (scene_driver.c, which runs a Lua scene script). */

#define SCENE_MAP_PORT 5121
#define SCENE_FRAME_MS 16

void scene_clock_advance(uint32_t ms);

#ifdef _WIN32
/* scene_input_win.c; the POSIX client declares them in platform/window.h. */
void platform_note_key(int vk, int down);
void platform_note_cursor(int x, int y);
#endif

/* Server side of the fake sockets. */
typedef void (*SceneServerPacket)(int port, const unsigned char *packet, int length);
void scene_net_set_handler(SceneServerPacket handler);
int scene_net_deliver(int port, const void *data, int size);
int scene_net_connected(int port);

#endif
