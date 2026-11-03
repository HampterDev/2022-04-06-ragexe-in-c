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
const char *session_get_state_name(struct Session *self, int state);
void session_get_monster_talk_info(struct Session *self, int job, int state, int value, char *out,
                                           size_t out_size, int aid);
void session_get_pet_speak(struct Session *self, int petJob, int hunger, int action, char *out,
                           size_t out_size);
void session_format_chat_line(struct Session *self, const char *text, char *out, size_t out_size, int aid,
                              int replaceWords);

int session_pet_hunger_level(int fullness);
int session_pet_intimacy_level(int intimacy);
int session_get_pet_emotion(struct Session *self, int hungerLevel, int intimacyLevel, int action);
int session_make_pet_talk_id(int job, int action, int hunger);
int session_get_sex(struct Session *self);
void session_set_sex(struct Session *self, int sex);
const char *session_get_pet_act_text(int action);
void session_get_pet_intimacy_text(int intimacy, char *out);
void session_get_pet_hunger_text(int fullness, char *out);
int session_get_pet_food(struct Session *self, int petJob);
int session_get_pet_job_by_egg(struct Session *self, int eggItemId);
const char *session_get_pet_string(struct Session *self, int petJob);
void session_get_pet_illust_name(struct Session *self, int petJob, char *out, size_t outSize);
const char *session_get_pet_accessory_act(struct Session *self, int accessory);
const char *session_get_pet_name(int petJob);

void session_append_prohibit(struct Session *self, int aid, unsigned minutes);
void session_erase_prohibit(struct Session *self, int aid, unsigned minutes);
int session_count_prohibits_of(struct Session *self, int aid);

void session_load_prohibition_log(struct Session *self);

#endif
