#include "contents/settings_json.h"
#include "json/service_json.h"
#include "core/text_encoding.h"
#include "platform/win_types.h"
#include <stdlib.h>
#include <string.h>

static int convert(struct JsonTree *data, unsigned from, unsigned to, unsigned depth)
{
    size_t i;
    if (depth > 16) return 0;
    if (data->type == JSON_TREE_STRING) {
        wchar_t *wide;
        char *text;
        if ((data->count > 16384 || memchr(data->u.text, 0, data->count)) ||
            !MultiByteToWideChar(from, MB_ERR_INVALID_CHARS, data->u.text, -1, NULL, 0)) return 0;
        wide = encoding_to_wide(from, data->u.text);
        if (!wide) return 0;
        text = encoding_from_wide(to, wide);
        free(wide);
        if (!text) return 0;
        free(data->u.text);
        data->u.text = text;
        data->count = strlen(text);
    } else if (data->type == JSON_TREE_ARRAY || data->type == JSON_TREE_OBJECT) {
        for (i = 0; i < data->count; ++i)
            if (!convert(data->type == JSON_TREE_ARRAY ? &data->u.items[i] : &data->u.members[i].value,
                         from, to, depth + 1)) return 0;
    }
    return 1;
}

int settings_json_convert(struct JsonTree *data, unsigned from, unsigned to)
{
    return convert(data, from, to, 0);
}
