#ifndef SKILLS_SKILL_BUNDLE_H
#define SKILLS_SKILL_BUNDLE_H

#include "skills/skill_bundle_types.h"
#include "skills/skill_answer.h"

struct PlayerSkillBundle *player_skill_bundle_construct(struct PlayerSkillBundle *self);
void player_skill_bundle_destruct(struct PlayerSkillBundle *self);

void player_skill_bundle_append_skill(struct PlayerSkillBundle *self,
                                               const struct SkillInfo *info);
struct SkillInfo *player_skill_bundle_find_skill(struct PlayerSkillBundle *self, int skill_id);
struct SkillInfo *skill_table_find(struct PlayerSkillBundle *self, struct SkillInfo *out,
                                             int skill_id);
void player_skill_bundle_remove_skill(struct PlayerSkillBundle *self, int skill_id);

void player_skill_bundle_add_tree_skill(struct PlayerSkillBundle *self, int job,
                                                const struct SkillInfo *info);
void player_skill_bundle_clear_trees(struct PlayerSkillBundle *self);
struct SkillInfo *player_skill_bundle_find_tree_skill(struct PlayerSkillBundle *self,
                                                              int skill_id);
int player_skill_bundle_tree_skill_level(struct PlayerSkillBundle *self, int skill_id);
struct List *player_skill_bundle_tree_tab(struct PlayerSkillBundle *self, int job);
unsigned player_skill_bundle_tree_tab_size(struct PlayerSkillBundle *self, int job);

void player_skill_bundle_add_extra_skill(struct PlayerSkillBundle *self,
                                                 const struct SkillInfo *info);
void player_skill_bundle_clear_extra_skills(struct PlayerSkillBundle *self);
unsigned char player_skill_bundle_remove_extra_skill(struct PlayerSkillBundle *self,
                                                             int skill_id);
struct SkillInfo *player_skill_bundle_find_extra_skill(struct PlayerSkillBundle *self,
                                                               int skill_id);
void player_skill_bundle_number_extra_skills(struct PlayerSkillBundle *self);

unsigned char player_skill_bundle_put_autorun_skill(struct PlayerSkillBundle *self,
                                                            const struct SkillInfo *info);
struct SkillInfo *player_skill_bundle_copy_autorun_skill(struct PlayerSkillBundle *self,
                                                                 struct SkillInfo *out, int skill_id);

struct SkillActionAnswer;
void player_skill_bundle_note_level_raise(struct PlayerSkillBundle *self,
                                                  const struct SkillInfo *info);
void player_skill_bundle_reconcile_level(struct PlayerSkillBundle *self,
                                                  struct SkillInfo *info);
struct SkillActionAnswer *player_skill_bundle_level_before_change(struct PlayerSkillBundle *self,
                                                                          struct SkillActionAnswer *out,
                                                                          int skill_id);
unsigned char player_skill_bundle_is_level_boosted(struct PlayerSkillBundle *self,
                                                           int skill_id);
void player_skill_bundle_clear_level_changes(struct PlayerSkillBundle *self);
void player_skill_bundle_zero_first_level_before(struct PlayerSkillBundle *self);

void player_skill_bundle_set_chosen_level(struct PlayerSkillBundle *self, int skill_id,
                                                  int level);

#endif
