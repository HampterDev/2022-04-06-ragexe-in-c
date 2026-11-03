#ifndef EFFECTS_DURATIONS_H
#define EFFECTS_DURATIONS_H

enum { EFFECT_DURATION_COUNT = 2385 };

extern const int g_effectDurations[EFFECT_DURATION_COUNT];

/* Frames an effect lasts. IDs past the table are unconfigured; like the
   unconfigured IDs inside it, they last as many frames as their ID. */
int effect_duration(int effect_id);

#endif
