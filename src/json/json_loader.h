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

#define JSON_MSG_NOT_LOADED "\xec\xb4\x88\xea\xb8\xb0\xed\x99\x94\xea\xb0\x80 \xeb\x90\x98\xec\x96\xb4\xec\x95\xbc \xed\x95\xa9\xeb\x8b\x88\xeb\x8b\xa4." /* "초기화가 되어야 합니다." */
#define JSON_MSG_NO_TABLE   "\xed\x82\xa4\xec\x97\x90 \xed\x95\xb4\xeb\x8b\xb9\xed\x95\x98\xeb\x8a\x94 \xeb\x8d\xb0\xec\x9d\xb4\xed\x84\xb0\xea\xb0\x80 " /* "키에 해당하는 데이터가 " */ \
                            "\xec\x97\x86\xec\x8a\xb5\xeb\x8b\x88\xeb\x8b\xa4. (key : %s)" /* "없습니다. (key : %s)" */
#define JSON_MSG_NO_FIELD   "key \xec\x97\x90 \xed\x95\xb4\xeb\x8b\xb9\xed\x95\x98\xeb\x8a\x94 \xeb\x8d\xb0\xec\x9d\xb4\xed\x84\xb0\xeb\xa5\xbc " /* "key 에 해당하는 데이터를 " */ \
                            "\xec\xb0\xbe\xec\xa7\x80 \xeb\xaa\xbb\xed\x96\x88\xec\x8a\xb5\xeb\x8b\x88\xeb\x8b\xa4. (keyName : %s)" /* "찾지 못했습니다. (keyName : %s)" */
#define JSON_MSG_DUPLICATE  "\xec\xa4\x91\xeb\xb3\xb5\xed\x82\xa4\xea\xb0\x80 \xec\x9e\x85\xeb\xa0\xa5\xeb\x90\x98\xec\x97\x88\xec\x8a\xb5\xeb\x8b\x88\xeb\x8b\xa4. " /* "중복키가 입력되었습니다. " */ \
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
