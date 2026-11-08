#include "skill_oracle.h"
#include "custom_removed.h"

#define SKILL_LIMIT 0x2200
#define MAX_REPORTED 60

typedef void (__fastcall *show_skill_use_fn)(void *self, void *edx, int skill, int level, int src_aid, int target_aid, int result);

static int call_original(show_skill_use_fn f, int skill, int level, int src, int target, int result)
{
    __try { f(orig_mode, NULL, skill, level, src, target, result); return 1; }
    __except (note_fault(GetExceptionInformation())) { return 0; }
}

static unsigned removed_skills;

int main(int argc, char **argv)
{
    const char *exe = argc > 1 ? argv[1] : getenv("ORACLE_EXE");
    static const int levels[] = { 1, 5, 10 };
    static const struct { int src, target, result; const char *name; } variants[] = {
        { AID_CASTER, AID_TARGET, 1, "C->T" },
        { AID_OWN, AID_TARGET, 1, "P->T" },
        { AID_CASTER, AID_CASTER, 1, "C->C" },
        { AID_CASTER, AID_TARGET, 0, "C->T no result" },
    };
    show_skill_use_fn orig;
    int skill, li, vi, option, compared = 0;
    unsigned mismatches = 0, faults = 0, reported = 0, recorded = 0, skills_with_events = 0;
    const int verbose = getenv("ORACLE_VERBOSE") != NULL;

    if (!exe) {
        fprintf(stderr, "usage: test_skill_use <original exe>\n");
        return 2;
    }
    if (!oracle_load(exe))
        return 2;
    skill_oracle_setup(verbose);
    orig = (show_skill_use_fn)oracle_addr(ORIG_SHOW_SKILL_USE);

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
                    int level = levels[li];

                    compared++;
                    if (verbose) { fprintf(stderr, "option %d skill 0x%x level %d %s\n", option, skill, level, variants[vi].name); fflush(stderr); }
                    seed_all(option);
                    event_count = 0;
                    overflow = 0;
                    srand(1);
                    if (!call_original(orig, skill, level, variants[vi].src, variants[vi].target, variants[vi].result)) {
                        faults++;
                        if (reported++ < MAX_REPORTED)
                            printf("  option %d skill 0x%x level %d %s: the original faulted (code %08x at 0x%x)\n", option, skill, level,
                                   variants[vi].name, fault_code, fault_va);
                        continue;
                    }
                    recorded += event_count;
                    any |= event_count > 0;
                    joined(orig_text, sizeof orig_text);

                    seed_all(option);
                    event_count = 0;
                    overflow = 0;
                    srand(1);
                    game_mode_show_skill_use(&lift_mode, (unsigned short)skill, level, variants[vi].src, variants[vi].target,
                                             (char)variants[vi].result);
                    joined(lift_text, sizeof lift_text);

                    if (strcmp(orig_text, lift_text) != 0) {
                        mismatches++;
                        if (reported++ < MAX_REPORTED)
                            printf("  option %d skill 0x%x level %d %s:\n    original: %s\n    lift:     %s\n", option, skill, level,
                                   variants[vi].name, orig_text, lift_text);
                    }
                }
            }
            skills_with_events += any;
        }
    }
    printf("  %u skill inputs left out: removed on custom (custom_removed.h)\n", removed_skills);
    printf("Skill use: %d calls compared against the original (%u events recorded, %u skill/option pairs with events), "
           "%u mismatches, %u faults%s\n", compared, recorded, skills_with_events, mismatches, faults,
           reported > MAX_REPORTED ? " (first 60 shown)" : "");
    return mismatches || faults ? 1 : 0;
}
