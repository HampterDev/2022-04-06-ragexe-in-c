#ifndef SESSION_NUMBER_CODEC_H
#define SESSION_NUMBER_CODEC_H

#include "base.h"
#include "std/string.h"
struct Session;

struct String *session_scramble_number(struct Session *self, struct String *out,
                                                     unsigned value);

void session_class_from_code(struct Session *self, int *out, int code);

BYTE session_is_number_text(struct Session *self, char *text);

BYTE session_has_control_char(struct Session *self, struct String text);

#endif
