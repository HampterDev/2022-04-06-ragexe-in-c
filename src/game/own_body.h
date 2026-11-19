#ifndef GAME_OWN_BODY_H
#define GAME_OWN_BODY_H

#include <stddef.h>
#include "platform/win_types.h"

/* The player's own body as the server has it, and the predicted world kept from it
   (world/predicted.h, plans/predicted-world.md): the server's word of the body (ZC_OWN_BODY)
   is taken, the player's requests go into the world as they leave for the server, the bodies
   the own one depends on (whom it is to attack or use a skill on, attacks or swings at) are
   kept in it as the client sees them, and the world runs to the own clock once a frame. What
   the character does ahead of the server is drawn from it and from nothing else: the body's
   move goes into the character's move track whenever it is another than the one drawn, and
   the swing the body has in its windup is the swing shown (game_mode_own_body_swing). So a
   click is drawn in its frame, the walk into reach of an attack, a skill or a pickup and the
   swings begin when the server's do, and a word of the server that is not what was predicted
   is one correction: the character is steered to where the body is, and a swing the body has
   no more is taken back. Until the first word comes the world is empty, and nothing runs. */

struct GameMode;

/* ZC_OWN_BODY, `size` bytes as they came. */
void game_mode_zc_own_body(struct GameMode *self, const unsigned char *pkt, size_t size);
/* The number of the player's placement on the map it is on (ZC_HOLD_BACK): the server's words
   of that placement are taken from now on, and of no other. */
void game_mode_own_body_placed(struct GameMode *self, unsigned placement);
/* A request to walk to the cell (x, y) left for the server, with its stamp's number. */
void game_mode_own_body_walk(struct GameMode *self, unsigned number, int x, int y);
/* An intent left for the server (CZ_INTENT, `kind` INTENT_*), with its stamp's number: an order
   to attack `target` in `mode`, the skill `skill_id` at `skill_lv` on `target` or on the cell
   (x, y), the pickup of the item `target`, or the release of a held attack. */
void game_mode_own_body_intent(struct GameMode *self, unsigned number, int kind, int mode, unsigned target, int x, int y,
                               int skill_id, int skill_lv);
/* Once a frame: the other bodies are said as they are seen, the world runs to the own clock,
   and the character is given the body's move when that is news. */
void game_mode_own_body_frame(struct GameMode *self);
/* A record of the world came (a unit's track, a hit, a coming or a going): a word of the own
   body that differs from what was predicted has something to explain it (predicted_heard). */
void game_mode_own_body_heard(DWORD tick);
/* The body `id` went from the predicted world at the server tick `tick` (ZC_VIEW_STAMP). */
void game_mode_own_body_gone(unsigned id, DWORD tick);
/* The swing the own body has in its windup, as the world has it now: at whom, when it began
   and lands (ticks of the wire), and the body's attack motion. Returns 0 when it has none.
   What is shown of it follows this once a frame (game_mode_own_swing). */
int game_mode_own_body_swing(unsigned *target, DWORD *at_tick, DWORD *lands, unsigned *motion_ms);
/* Whether a walk the player asked for still waits for the server's word. */
int game_mode_own_body_walk_waits(void);
/* The own body as predicted: its bytes (BODY_STATE_SIZE) and the server's tick they are of, as
   the wire has ticks. Returns 0 while nothing is predicted. */
int game_mode_own_body(DWORD *tick, unsigned char *state);

#endif
