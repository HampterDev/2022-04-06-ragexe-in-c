#include "skill_oracle.h"
#include "custom_removed.h"
#include "game/packet_handlers.h"

#define ORIG_NOTIFY_SKILL2 0x801150u
#define SKILL_LIMIT 0x2200
#define MAX_REPORTED 60

#pragma pack(push, 1)
struct PacketNotifySkill2 {
    unsigned short packet_type;
    unsigned short skill_id;
    int src_aid;
    int target_aid;
    int start_time;
    int attack_mt;
    int attacked_mt;
    int damage;
    short level;
    short count;
    unsigned char action;
};
#pragma pack(pop)

typedef void (__fastcall *notify_skill2_fn)(void *self, void *edx, const struct PacketNotifySkill2 *packet);

static int call_original(notify_skill2_fn f, const struct PacketNotifySkill2 *packet)
{
    __try { f(orig_mode, NULL, packet); return 1; }
    __except (note_fault(GetExceptionInformation())) { return 0; }
}

static unsigned removed_skills;

int main(int argc, char **argv)
{
    const char *exe = argc > 1 ? argv[1] : getenv("ORACLE_EXE");
    static const int levels[] = { 1, 5, 10 };
    static const struct { int src, target; int damage; unsigned char action; const char *name; } variants[] = {
        { AID_CASTER, AID_TARGET, 150, 8, "C->T hit" },
        { AID_CASTER, AID_TARGET, 0, 8, "C->T miss" },
        { AID_OWN, AID_TARGET, 150, 8, "P->T hit" },
        { AID_CASTER, AID_OWN, 150, 8, "C->P hit" },
        { AID_CASTER, AID_TARGET, 150, 6, "C->T crit" },
        { 5555, AID_TARGET, 150, 8, "unknown->T" },
    };
    notify_skill2_fn orig;
    int skill, li, vi, option, compared = 0;
    unsigned mismatches = 0, faults = 0, reported = 0, recorded = 0, skills_with_events = 0;
    const int verbose = getenv("ORACLE_VERBOSE") != NULL;

    if (!exe) {
        fprintf(stderr, "usage: test_skill_hit <original exe>\n");
        return 2;
    }
    if (!oracle_load(exe))
        return 2;
    skill_oracle_setup(verbose);
    orig = (notify_skill2_fn)oracle_addr(ORIG_NOTIFY_SKILL2);

    for (option = 0; option < 2; option++) {
        for (skill = 0; skill < SKILL_LIMIT; skill++) {
            if (custom_removed_skill(skill)) {
                removed_skills++;
                continue;
            }
            int any = 0;
            for (li = 0; li < (int)(sizeof levels / sizeof *levels); li++) {
                for (vi = 0; vi < (int)(sizeof variants / sizeof *variants); vi++) {
                    char orig_text[EVENTS * EVENT_LEN], lift_text[EVENTS * EVENT_LEN];
                    struct PacketNotifySkill2 packet;

                    memset(&packet, 0, sizeof packet);
                    packet.packet_type = 0x1de;
                    packet.skill_id = (unsigned short)skill;
                    packet.src_aid = variants[vi].src;
                    packet.target_aid = variants[vi].target;
                    packet.start_time = 123456;
                    packet.attack_mt = 500;
                    packet.attacked_mt = 400;
                    packet.damage = variants[vi].damage;
                    packet.level = (short)levels[li];
                    packet.count = 3;
                    packet.action = variants[vi].action;

                    compared++;
                    if (verbose) { fprintf(stderr, "option %d skill 0x%x level %d %s\n", option, skill, levels[li], variants[vi].name); fflush(stderr); }
                    seed_all(option);
                    event_count = 0;
                    overflow = 0;
                    srand(1);
                    if (!call_original(orig, &packet)) {
                        faults++;
                        if (reported++ < MAX_REPORTED)
                            printf("  option %d skill 0x%x level %d %s: the original faulted (code %08x at 0x%x)\n", option, skill,
                                   levels[li], variants[vi].name, fault_code, fault_va);
                        continue;
                    }
                    recorded += event_count;
                    any |= event_count > 0;
                    joined(orig_text, sizeof orig_text);

                    seed_all(option);
                    event_count = 0;
                    overflow = 0;
                    srand(1);
                    game_mode_on_notify_skill2(&lift_mode, &packet);
                    joined(lift_text, sizeof lift_text);

                    if (strcmp(orig_text, lift_text) != 0) {
                        mismatches++;
                        if (reported++ < MAX_REPORTED)
                            printf("  option %d skill 0x%x level %d %s:\n    original: %s\n    lift:     %s\n", option, skill, levels[li],
                                   variants[vi].name, orig_text, lift_text);
                    }
                }
            }
            skills_with_events += any;
        }
    }
    printf("  %u skill inputs left out: removed on custom (custom_removed.h)\n", removed_skills);
    printf("Skill hit: %d calls compared against the original (%u events recorded, %u skill/option pairs with events), "
           "%u mismatches, %u faults%s\n", compared, recorded, skills_with_events, mismatches, faults,
           reported > MAX_REPORTED ? " (first 60 shown)" : "");
    return mismatches || faults ? 1 : 0;
}
