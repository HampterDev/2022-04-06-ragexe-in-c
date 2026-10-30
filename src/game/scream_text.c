#include "game/scream_text.h"

int scream_text_line(char *out, size_t capacity, const char *file, size_t bytes, unsigned wanted)
{
    unsigned line = 0;
    size_t i, used = 0;

    if (!capacity)
        return 0;
    out[0] = 0;
    for (i = 0; i < bytes; ++i) {
        char c = file[i];
        if (c == '\r') {
            if (line == wanted)
                break;
            ++line;
        } else if (line == wanted && c != '\n' && c != '\t') {
            if (used == capacity - 1) {
                out[0] = 0;
                return 0;
            }
            out[used++] = c;
        }
    }
    out[used] = 0;
    return line == wanted;
}
