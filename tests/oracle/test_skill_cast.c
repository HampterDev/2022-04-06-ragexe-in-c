#include "skill_oracle.h"
#include "game/skill_cast.h"

#define ORIG_USESKILL_ACK 0x7eb5f0u
#define SKILL_LIMIT 0x2200
#define MAX_REPORTED 60

typedef void (__fastcall *useskill_ack_fn)(void *self, void *edx, unsigned src_aid, unsigned target_aid, int x, int y, int skill,
                                           int property, unsigned delay_ms, int is_disposable, int attack_motion);

static int call_original(useskill_ack_fn f, unsigned src, unsigned target, int x, int y, int skill, int property, unsigned delay,
                         int disposable, int motion)
{
    __try { f(orig_mode, NULL, src, target, x, y, skill, property, delay, disposable, motion); return 1; }
    __except (note_fault(GetExceptionInformation())) { return 0; }
}

int main(int argc, char **argv)
{
    const char *exe = argc > 1 ? argv[1] : getenv("ORACLE_EXE");
    static const struct { unsigned src, target; int x, y, property; unsigned delay; int disposable, motion; const char *name; } variants[] = {
        { AID_CASTER, AID_TARGET, 100, 120, 0, 1500, 0, 500, "C->T 1.5s" },
        { AID_CASTER, 0, 100, 120, 0, 2000, 0, 500, "C->cell 2s" },
        { AID_OWN, AID_TARGET, 100, 120, 0, 0, 1, 500, "P->T instant disposable" },
        { AID_CASTER, AID_CASTER, 100, 120, 3, 3000, 0, 500, "C->C 3s property 3" },
        { AID_CASTER, AID_OWN, 0, 0, 0, 800, 0, 0, "C->P 0.8s" },
    };
    useskill_ack_fn orig;
    int skill, vi, option, compared = 0;
    unsigned mismatches = 0, faults = 0, reported = 0, recorded = 0, skills_with_events = 0;
    const int verbose = getenv("ORACLE_VERBOSE") != NULL;

    if (!exe) {
        fprintf(stderr, "usage: test_skill_cast <original exe>\n");
        return 2;
    }
    if (!oracle_load(exe))
        return 2;
    skill_oracle_setup(verbose);
    orig = (useskill_ack_fn)oracle_addr(ORIG_USESKILL_ACK);

    for (option = 0; option < 2; option++) {
        for (skill = 0; skill < SKILL_LIMIT; skill++) {
            int any = 0;
            for (vi = 0; vi < (int)(sizeof variants / sizeof *variants); vi++) {
                char orig_text[EVENTS * EVENT_LEN], lift_text[EVENTS * EVENT_LEN];

                compared++;
                if (verbose) { fprintf(stderr, "option %d skill 0x%x %s\n", option, skill, variants[vi].name); fflush(stderr); }
                seed_all(option);
                event_count = 0;
                overflow = 0;
                srand(1);
                if (!call_original(orig, variants[vi].src, variants[vi].target, variants[vi].x, variants[vi].y, skill, variants[vi].property,
                                   variants[vi].delay, variants[vi].disposable, variants[vi].motion)) {
                    faults++;
                    if (reported++ < MAX_REPORTED)
                        printf("  option %d skill 0x%x %s: the original faulted (code %08x at 0x%x)\n", option, skill, variants[vi].name,
                               fault_code, fault_va);
                    continue;
                }
                recorded += event_count;
                any |= event_count > 0;
                joined(orig_text, sizeof orig_text);

                seed_all(option);
                event_count = 0;
                overflow = 0;
                srand(1);
                game_mode_zc_useskill_ack(&lift_mode, variants[vi].src, variants[vi].target, variants[vi].x, variants[vi].y, skill,
                                          variants[vi].property, variants[vi].delay, (unsigned char)variants[vi].disposable,
                                          variants[vi].motion);
                joined(lift_text, sizeof lift_text);

                if (strcmp(orig_text, lift_text) != 0) {
                    mismatches++;
                    if (reported++ < MAX_REPORTED)
                        printf("  option %d skill 0x%x %s:\n    original: %s\n    lift:     %s\n", option, skill, variants[vi].name,
                               orig_text, lift_text);
                }
            }
            skills_with_events += any;
        }
    }
    printf("Skill cast: %d calls compared against the original (%u events recorded, %u skill/option pairs with events), "
           "%u mismatches, %u faults%s\n", compared, recorded, skills_with_events, mismatches, faults,
           reported > MAX_REPORTED ? " (first 60 shown)" : "");
    return mismatches || faults ? 1 : 0;
}
