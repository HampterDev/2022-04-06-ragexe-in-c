/* custom_removed.h - the job and skill IDs that custom removed on purpose, so the oracle tests
 * leave them out of the comparison with the original. The ranges are the ones the removals were
 * made with (docs/custom-review.md: "4th-class removal (2026-11-02)" and "Third-class removal
 * (2026-11-03)"); every other ID must still answer as the original does. */
#ifndef ORACLE_CUSTOM_REMOVED_H
#define ORACLE_CUSTOM_REMOVED_H

static int custom_in(int id, int first, int last)
{
    return id >= first && id <= last;
}

static int custom_removed_job(int job)
{
    static const int third_mounts[] = {
        0x1018, 0x101f, 0x1021, 0x1024, 0x1025, 0x102c, 0x102f, 0x1034, 0x1037, 0x1056,
        0x1065, 0x1066, 0x1069, 0x106a, 0x106d, 0x106e, 0x1070, 0x1071,
    };
    static const int expanded[] = {
        0x107f, 0x1080, 0x1085, 0x1087, 0x1088, 0x108d,
    };
    size_t i;

    if (custom_in(job, 0xfd6, 0x1010) || custom_in(job, 0x104d, 0x1054))
        return 1;
    for (i = 0; i < sizeof third_mounts / sizeof third_mounts[0]; i++)
        if (job == third_mounts[i])
            return 1;
    if (custom_in(job, 0x105e, 0x1061) || custom_in(job, 0x1073, 0x107d) || custom_in(job, 0x108f, 0x1098))
        return 1;
    for (i = 0; i < sizeof expanded / sizeof expanded[0]; i++)
        if (job == expanded[i])
            return 1;
    return custom_in(job, 0x109c, 0x10cb) || custom_in(job, 0x10ce, 0x10e9);
}

static int custom_removed_skill(int skill)
{
    if (custom_in(skill, 2001, 2532))
        return skill != 2285;
    return custom_in(skill, 2551, 2612) || custom_in(skill, 3001, 3029) || custom_in(skill, 5001, 5057) ||
           custom_in(skill, 5072, 5073) || custom_in(skill, 0x1451, 0x157a) || custom_in(skill, 8018, 8042) ||
           custom_in(skill, 8045, 8060) || custom_in(skill, 8401, 8442) || custom_in(skill, 0x20fb, 0x2109) ||
           custom_in(skill, 0x2199, 0x219d);
}

#endif
