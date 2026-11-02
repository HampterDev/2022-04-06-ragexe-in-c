#ifndef TESTS_SCENES_SCENE_H
#define TESTS_SCENES_SCENE_H

#include <stddef.h>
#include <stdint.h>

/* The scene harness is the whole client with four files swapped: the clock (scene_clock.c), the
   sockets (scene_net.c, an in-process map server), the threads (scene_thread.c) and the per-frame
   driver hook (scene_driver.c, which runs a Lua scene script). */

#define SCENE_MAP_PORT 5121
#define SCENE_FRAME_MS 16

void scene_clock_advance(uint32_t ms);

/* Server side of the fake sockets. */
typedef void (*SceneServerPacket)(int port, const unsigned char *packet, int length);
void scene_net_set_handler(SceneServerPacket handler);
int scene_net_deliver(int port, const void *data, int size);
int scene_net_connected(int port);

#endif
