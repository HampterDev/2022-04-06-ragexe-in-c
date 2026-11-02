#ifndef CORE_ONCE_H
#define CORE_ONCE_H

struct Once {
    void *state;
};

int once_begin(struct Once *once);
void once_complete(struct Once *once);

#endif
