#ifndef WORLD_OBJECT_LOOKUP_H
#define WORLD_OBJECT_LOOKUP_H
struct World;
struct Item;
struct Skill;
struct Item *world_get_item_by_aid(struct World *world, int aid);
struct Skill *world_get_skill_by_aid(struct World *world, int aid);
#endif
