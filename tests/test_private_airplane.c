#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game/connection.h"
#include "game/packet_handlers.h"
#include "session/state.h"
#include "std/string.h"

/* Run the production private-airplane map and server moves on the 16-byte map field
   of their packets, followed by bytes that are not NUL. A full field has no NUL; it must
   reach the map move and the server move whole, and nothing after it. */

struct Session g_session1;
static struct GameMode *const test_mode = (struct GameMode *)0x6d6f;
static unsigned char moved[0x40];
static char server_map[0x40];
static int map_moves, server_moves, cases;

__declspec(noreturn) void fatal_error(const char *message) { (void)message; abort(); }

void game_mode_zc_npcack_mapmove(struct GameMode *self, const unsigned char *pkt)
{
    assert(self == test_mode);
    memcpy(moved, pkt, 2 + 16 + 4);
    ++map_moves;
}

void game_mode_move_to_server(struct GameMode *self, struct String map_name, int x, int y,
                              unsigned long ip, int port, struct String server_map_name)
{
    assert(self == test_mode && x == 12 && y == 34 && ip == 0x0100007f && port == 5121);
    assert(server_map_name.size == 0 && map_name.size < sizeof server_map);
    memcpy(server_map, string_cstr(&map_name), map_name.size + 1);
    ++server_moves;
    string_free(&map_name);
    string_free(&server_map_name);
}

#include "private_airplane.inc"

/* The packet's map field holding name, then 'Z' bytes. */
static const char *field(char *buffer, size_t size, const char *name)
{
    memset(buffer, 'Z', size - 1);
    buffer[size - 1] = 0;
    memcpy(buffer, name, strlen(name) < 16 ? strlen(name) + 1 : 16);
    return buffer;
}

static void check(const char *name, const char *expected)
{
    char buffer[0x30], read[17];
    int before_map = map_moves, before_server = server_moves;

    g_session1.loadingAirplane = 0;
    memset(moved, 0x55, sizeof moved);
    game_mode_on_map_move_private_airplane(test_mode, field(buffer, sizeof buffer, name), 12, 34);
    assert(map_moves == before_map + 1 && g_session1.loadingAirplane == 1);
    /* As game_mode_zc_npcack_mapmove reads the field. */
    assert(moved[0] == 0x91 && moved[1] == 0);
    memcpy(read, moved + 2, 16);
    read[16] = 0;
    assert(strcmp(read, expected) == 0);
    assert(moved[18] == 12 && moved[19] == 0 && moved[20] == 34 && moved[21] == 0);

    g_session1.loadingAirplane = 0;
    game_mode_on_server_move_private_airplane(test_mode, field(buffer, sizeof buffer, name), 12, 34,
                                              0x0100007f, 5121);
    assert(server_moves == before_server + 1 && g_session1.loadingAirplane == 1);
    assert(strcmp(server_map, expected) == 0);
    ++cases;
}

int main(void)
{
    check("prontera.gat", "prontera.gat");
    check("airport", "airport");
    check("", "");
    check("abcdefghijk.gat", "abcdefghijk.gat");
    check("abcdefghijkl.gat", "abcdefghijkl.gat");
    printf("Private airplane: %d map names; a full 16-byte field is read whole and no further.\n", cases);
    return 0;
}
