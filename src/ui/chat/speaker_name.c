#include "ui/chat/speaker_name.h"
#include <string.h>

void chat_speaker_name(char name[25], const char *text, int entered, int left, int colon, int arabic)
{
    size_t length = strlen(text), offset, count;
    int mark = entered > 0 && (size_t)entered < length ? entered : left;

    name[0] = 0;
    if (mark > 0 && (size_t)mark < length) {
        offset = arabic ? (size_t)mark + 1 : 1;
        count = arabic ? length - offset : (size_t)mark - 1;
    } else if (colon > 0 && colon < 24 && (size_t)colon < length) {
        offset = arabic ? (size_t)colon : 0;
        count = arabic ? length - offset : (size_t)colon;
    } else {
        return;
    }
    if (count >= 25)
        return;
    memcpy(name, text + offset, count);
    name[count] = 0;
}
