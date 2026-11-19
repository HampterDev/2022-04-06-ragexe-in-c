#ifndef TESTS_PLAYGROUND_PLAY_H
#define TESTS_PLAYGROUND_PLAY_H

#include <stddef.h>

/* The playground is the scene harness (tests/scenes) with its driver swapped: play_driver.c
   runs no script and plays no map server. It hands what the client sends to a runner outside
   the process, delivers what the runner sends back, does what the runner tells the player to
   do, and reports where every actor is drawn, once a frame, over play_link.c. The runner hosts
   the real server on the same virtual clock (the ring-buffer worktree's
   consumers/host/playground). tests/playground/README.md has the messages. */

/* The line to the runner, on this machine's `port`. Each returns 0 when the line is gone. */
int play_link_open(int port);
int play_link_send(const void *data, size_t size);
int play_link_receive(void *data, size_t size);

#endif
