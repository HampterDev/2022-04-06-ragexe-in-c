#ifndef CORE_OWNED_TEXT_H
#define CORE_OWNED_TEXT_H

#include "core/text_view.h"

struct OwnedText {
    char *data;
    size_t size;
};

void owned_text_init(struct OwnedText *text);
void owned_text_clear(struct OwnedText *text);
int owned_text_assign(struct OwnedText *text, struct TextView source);
int owned_text_concat(struct OwnedText *text, struct TextView first, struct TextView second);
const char *owned_text_cstr(const struct OwnedText *text);
struct TextView owned_text_view(const struct OwnedText *text);

#endif
