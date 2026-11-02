#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "json/json_loader.h"
#include "game/client.h"

enum {
    JSON_ERROR_BUFFER = 0x800,
};

void json_loader_init(struct JsonCppLoader *self)
{
    json_tree_init(&self->root);
    string_init(&self->error);
    self->loaded = 0;
    self->ok = 1;
}

void json_loader_free(struct JsonCppLoader *self)
{
    string_free(&self->error);
    json_tree_free(&self->root);
}

#define MSG_LOAD_EMPTY      "error : JsonCppLoader::Load failed (reason : " \
                            "비어있는 버퍼가 전달되었습니다.)"
#define MSG_BSON_FAILED     "error : JsonCppLoader::LoadBson failed (reason : %s)"

unsigned char json_loader_load_bson(struct JsonCppLoader *self, const struct Vec *bytes)
{
    struct JsonTree doc;

    if (bytes->first == bytes->last) {
        json_loader_set_error(self, MSG_LOAD_EMPTY);
        return 0;
    }
    if (!json_tree_from_bson(&doc, (const unsigned char *)bytes->first,
                             (size_t)((const char *)bytes->last - (const char *)bytes->first))) {
        json_loader_set_error(self, MSG_BSON_FAILED, "BSON decode failed");
        return 0;
    }
    json_tree_free(&self->root);
    self->root = doc;
    self->loaded = 1;
    return 1;
}

const struct JsonTree *json_loader_member(const struct JsonTree *row, const char *key, size_t key_length, int type)
{
    const struct JsonTree *value = json_tree_find(row, key, key_length);
    int value_type;

    if (!value)
        return NULL;
    value_type = value->type == JSON_TREE_UINT64 ? JSON_TREE_INT64 : value->type;
    return value_type == type ? value : NULL;
}

void json_loader_set_error(struct JsonCppLoader *self, const char *fmt, ...)
{
    char text[JSON_ERROR_BUFFER];
    va_list args;

    memset(text, 0, sizeof(text));
    va_start(args, fmt);
    vsnprintf(text, sizeof(text), fmt, args);
    va_end(args);
    string_assign_cstr(&self->error, text);
    self->ok = 0;
}

static const struct JsonTree *field(struct JsonCppLoader *self, const char *key, const struct JsonTree *row, int type)
{
    const struct JsonTree *value = json_loader_member(row, key, strlen(key), type);

    if (!value)
        json_loader_set_error(self, JSON_MSG_NO_FIELD, key);
    return value;
}

void json_loader_read_string(struct JsonCppLoader *self, const char *key, const struct JsonTree *row,
                             struct String *out)
{
    const struct JsonTree *value = field(self, key, row, JSON_TREE_STRING);

    if (value)
        string_assign(out, value->u.text, value->count);
}

void json_loader_read_double(struct JsonCppLoader *self, const char *key, const struct JsonTree *row, double *out)
{
    const struct JsonTree *value = field(self, key, row, JSON_TREE_DOUBLE);

    if (value)
        *out = value->u.dbl;
}

void json_loader_read_bool(struct JsonCppLoader *self, const char *key, const struct JsonTree *row,
                           unsigned char *out)
{
    const struct JsonTree *value = field(self, key, row, JSON_TREE_BOOL);

    if (value)
        *out = value->boolean;
}

void json_loader_read_int(struct JsonCppLoader *self, const char *key, const struct JsonTree *row, int *out)
{
    const struct JsonTree *value = field(self, key, row, JSON_TREE_INT64);

    if (value)
        *out = (int)value->u.i64;
}

const struct JsonTree *json_loader_table(struct JsonCppLoader *self, const char *name, size_t name_length)
{
    const struct JsonTree *table;

    if (!self->loaded) {
        json_loader_set_error(self, JSON_MSG_NOT_LOADED);
        return NULL;
    }
    table = json_tree_find(&self->root, name, name_length);
    if (!table)
        json_loader_set_error(self, JSON_MSG_NO_TABLE, name);
    return table;
}
