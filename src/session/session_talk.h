#ifndef SESSION_SESSION_TALK_H
#define SESSION_SESSION_TALK_H

#include <stddef.h>

struct Session;
struct String;
struct Vec;
struct File;

struct ProhibitionRecord {
    int aid;
    int actor_id;
    unsigned minutes;
    int date;
    int time;
    char name[32];
    char giver[32];
};

void session_init_talk_type_table_(struct Session *self);

int session_get_talk_type(struct Session *self, const char *text, int *type, struct String *out);

void session_get_monster_skill_info(struct Session *self, const char *monsterName, int skillId,
                                            int *action, int *loopType);
struct String *session_get_monster_sprite_name(struct Session *self, struct String *out,
                                                          int job);

int session_pet_hunger_level(int fullness);
int session_pet_intimacy_level(int intimacy);
int session_get_pet_emotion(struct Session *self, int hungerLevel, int intimacyLevel, int action);
int session_get_sex(struct Session *self);
void session_set_sex(struct Session *self, int sex);
void session_get_pet_intimacy_text(int intimacy, char *out);
void session_get_pet_hunger_text(int fullness, char *out);
void session_get_pet_illust_name(struct Session *self, int petJob, char *out, size_t outSize);
/* Returns NULL when the accessory has no mapped, available animation. */
const char *session_get_pet_accessory_act(struct Session *self, int accessory);

void session_append_prohibit(struct Session *self, int aid, unsigned minutes);
void session_erase_prohibit(struct Session *self, int aid, unsigned minutes);
int session_count_prohibits_of(struct Session *self, int aid);

void session_load_prohibition_log(struct Session *self);

#endif
