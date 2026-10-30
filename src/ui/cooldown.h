#ifndef UI_COOLDOWN_H
#define UI_COOLDOWN_H

struct PieVertex {
    float x, y, z, rhw;
    unsigned color, specular;
    float u, v;
};
#define PIE_VERTICES 24
#define PIE_SEGMENTS 8

struct PieEdge { float base, edge; int reach, add; };
struct CooldownTimer {
    struct PieEdge edges[PIE_SEGMENTS];
    unsigned edge_mask;
    unsigned segment;
    int vertex_count;
    float angle;
    unsigned total_ms, start, elapsed;
    int last_minute, mode;
    struct PieVertex verts[PIE_VERTICES];
    int x, y, slot;
    unsigned color;
};

struct CooldownSlot { int skill_id, slot, x, y; };
struct CooldownSlotNode {
    struct CooldownSlotNode *next;
    struct CooldownSlot value;
};
struct CooldownSlotList { struct CooldownSlotNode *head; unsigned size; };
struct CooldownTimerNode {
    struct CooldownTimerNode *next, *prev;
    struct CooldownTimer value;
};
struct CooldownTimerList { struct CooldownTimerNode *head; unsigned size; };
struct SkillCooldown {
    int skill_id;
    unsigned end_time;
    int total_ms;
    struct CooldownTimerList timers_bar, timers_quick;
};
struct SkillCooldownNode {
    struct SkillCooldownNode *next, *prev;
    struct SkillCooldown value;
};
struct SkillCooldownList { struct SkillCooldownNode *head; unsigned size; };

int cooldown_slots_add(struct CooldownSlotList *list, int skill_id, int slot, int x, int y);
void cooldown_slots_clear(struct CooldownSlotList *list);
struct CooldownTimerNode *cooldown_timers_append(struct CooldownTimerList *list, const struct CooldownTimer *timer);
struct CooldownTimerNode *cooldown_timers_find(const struct CooldownTimerList *list, int slot);
struct CooldownTimerNode *cooldown_timers_remove(struct CooldownTimerList *list, struct CooldownTimerNode *node);
void cooldown_timers_clear(struct CooldownTimerList *list);
void skill_cooldown_clear(struct SkillCooldown *cooldown);
struct SkillCooldownNode *skill_cooldowns_find(const struct SkillCooldownList *list, int skill_id);
struct SkillCooldownNode *skill_cooldowns_append(struct SkillCooldownList *list, const struct SkillCooldown *value);
struct SkillCooldownNode *skill_cooldowns_remove(struct SkillCooldownList *list, struct SkillCooldownNode *node);
void skill_cooldowns_clear(struct SkillCooldownList *list);
int skill_cooldowns_clone(struct SkillCooldownList *to, const struct SkillCooldownList *from);
int skill_cooldowns_splice(struct SkillCooldownList *to, struct SkillCooldownList *from);

void cooldown_timer_init(struct CooldownTimer *timer, int slot, int x, int y,
    unsigned total_ms, unsigned start, unsigned elapsed_ms, int mode);
int cooldown_timer_layout(struct CooldownTimer *timer, unsigned now, int x, int y,
    int w, int h, int limit, int dx, int dy);
unsigned cooldown_clock_now(void);
int cooldown_time_pending(unsigned end, unsigned now);
int skill_cooldown_running(const struct SkillCooldown *cooldown);

#endif
