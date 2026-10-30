#ifndef SERVICE_JSON_H
#define SERVICE_JSON_H

#include "json/json_tree.h"
#include "core/owned_text.h"

void json_value_from_int(struct JsonTree *v, int value);
void json_value_from_bool(struct JsonTree *v, char value);
void json_value_from_text(struct JsonTree *v, struct TextView text);
void json_value_from_cstr(struct JsonTree *v, const char *text);
void json_value_copy(struct JsonTree *out, const struct JsonTree *source);
void json_value_assign(struct JsonTree *out, const struct JsonTree *source);
struct JsonTree *json_value_member(struct JsonTree *object, const char *key);
struct JsonTree *json_value_at(struct JsonTree *array, unsigned index);
struct JsonTree *json_value_get(const struct JsonTree *object, struct JsonTree *out,
                    const char *key, const struct JsonTree *fallback);
void json_value_append(struct JsonTree *array, const struct JsonTree *value);
struct JsonTree *json_value_child(struct JsonTree *parent, size_t index);
size_t json_value_size(const struct JsonTree *value);
int json_value_as_int(const struct JsonTree *value);
double json_value_as_double(const struct JsonTree *value);
char json_value_as_bool(const struct JsonTree *value);
struct TextView json_value_text(const struct JsonTree *value);
char json_value_is_null(const struct JsonTree *value);
char json_value_is_object(const struct JsonTree *value);
char json_value_empty(const struct JsonTree *value);

char json_parse_text(struct JsonTree *root, const char *text, size_t length);
int json_write_request(const struct JsonTree *root, struct OwnedText *out);

#endif
