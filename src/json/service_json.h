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
void json_value_append(struct JsonTree *array, const struct JsonTree *value);
struct TextView json_value_text(const struct JsonTree *value);

char json_parse_text(struct JsonTree *root, const char *text, size_t length);
int json_write_request(const struct JsonTree *root, struct OwnedText *out);

#endif
