#ifndef GAMEMODE_PACKET_DATA_H
#define GAMEMODE_PACKET_DATA_H

#include "game/gamemode.h"

struct Tree;

void packet_decode_move_data(const unsigned char *p, int *x0, int *y0, int *x1, int *y1, int *sx, int *sy);
int job_get_base_job(int job);
struct Tree *state_ids_by_value(struct Tree *out);
struct Tree *state_ids_by_flag(struct Tree *out);
void client_add_skill(unsigned short id, short level, int type, unsigned char upgradable, short sp_cost,
                               short range, short level2);
void client_post_aid_text(int aid, struct String text);

#endif
