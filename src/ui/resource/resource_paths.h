#ifndef UI_RESOURCE_PATHS_H
#define UI_RESOURCE_PATHS_H

struct String;

struct String *resource_path_accessory(struct String *out, int ext, int job, int sex, int hair, int accessory);
struct String *resource_path_body_palette(struct String *out, int job, int sex, int palette, int costume, int mount);
struct String *resource_path_body_sprite(struct String *out, int ext, int job, int body, int sex, int costume);
struct String *resource_path_head_palette(struct String *out, int hair, int job, int sex, int palette);
struct String *resource_path_head_sprite(struct String *out, int ext, int job, int sex, int *hair);
struct String *resource_path_imf(struct String *out, int job, int sex);
struct String *resource_path_job_body_sprite(struct String *out, int ext, int job, int sex, int costume, int mount);
struct String *resource_path_robe(struct String *out, int ext, int job, int sex, int robe, unsigned char map_job);
struct String *resource_path_shield(struct String *out, int ext, int job, int sex, int shield);
struct String *resource_path_weapon_glow(struct String *out, int ext, int job, int sex, int weapon, int view, int glow);
struct String *resource_path_weapon(struct String *out, int ext, int job, int sex, int weapon, int view);

#endif
