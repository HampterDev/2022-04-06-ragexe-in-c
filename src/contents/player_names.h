#ifndef CONTENTS_PLAYER_NAMES_H
#define CONTENTS_PLAYER_NAMES_H

#include "core/text_view.h"

int player_names_create(void);
void player_names_destroy(void);
void player_names_clear(void);

int player_names_set(unsigned char_id, struct TextView name);
int player_names_has(unsigned char_id);

struct TextView player_name_lookup(unsigned char_id);

void player_names_update(void);

#endif
