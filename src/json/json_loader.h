#ifndef JSON_LOADER_H
#define JSON_LOADER_H

#include "base.h"
#include "json/json_tree.h"

struct JsonCppLoader {
    struct JsonTree root;
    struct String error;
    unsigned char loaded;
    unsigned char ok;
};

#define JSON_MSG_NOT_LOADED "초기화가 되어야 합니다."
#define JSON_MSG_NO_TABLE   "키에 해당하는 데이터가 " \
                            "없습니다. (key : %s)"
#define JSON_MSG_NO_FIELD   "key 에 해당하는 데이터를 " \
                            "찾지 못했습니다. (keyName : %s)"
#define JSON_MSG_DUPLICATE  "중복키가 입력되었습니다. " \
                            "(key : %s)"

void json_loader_init(struct JsonCppLoader *self);
void json_loader_free(struct JsonCppLoader *self);
unsigned char json_loader_load_bson(struct JsonCppLoader *self, const struct Vec *bytes);
const struct JsonTree *json_loader_member(const struct JsonTree *row, const char *key, size_t key_length, int type);
void json_loader_set_error(struct JsonCppLoader *self, const char *fmt, ...);

void json_loader_read_string(struct JsonCppLoader *self, const char *key, const struct JsonTree *row,
                             struct String *out);
void json_loader_read_double(struct JsonCppLoader *self, const char *key, const struct JsonTree *row, double *out);
void json_loader_read_bool(struct JsonCppLoader *self, const char *key, const struct JsonTree *row,
                           unsigned char *out);
void json_loader_read_int(struct JsonCppLoader *self, const char *key, const struct JsonTree *row, int *out);

const struct JsonTree *json_loader_table(struct JsonCppLoader *self, const char *name, size_t name_length);

#endif
