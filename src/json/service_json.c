#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "json/service_json.h"
#include <yyjson.h>

static void *allocate(size_t count, size_t size)
{
    void *p;
    if (size && count > (size_t)-1 / size)
        abort();
    p = calloc(count, size);
    if (!p && count && size)
        abort();
    return p;
}

static char *copy_text(const char *text, size_t length)
{
    char *copy;
    if (length == (size_t)-1)
        abort();
    copy = allocate(length + 1, 1);
    memcpy(copy, text, length);
    return copy;
}

void json_value_from_int(struct JsonTree *v, int value)
{
    json_tree_init(v);
    v->type = JSON_TREE_INT64;
    v->u.i64 = value;
}

void json_value_from_bool(struct JsonTree *v, char value)
{
    json_tree_init(v);
    v->type = JSON_TREE_BOOL;
    v->boolean = value != 0;
}

static void from_text(struct JsonTree *v, const char *text, size_t length)
{
    json_tree_init(v);
    v->type = JSON_TREE_STRING;
    v->count = length;
    v->u.text = copy_text(text, length);
}

void json_value_from_text(struct JsonTree *v, struct TextView text)
{
    from_text(v, text.data, text.size);
}

void json_value_from_cstr(struct JsonTree *v, const char *text)
{
    from_text(v, text, strlen(text));
}

void json_value_copy(struct JsonTree *out, const struct JsonTree *source)
{
    size_t i;
    if (!source) {
        json_tree_init(out);
        return;
    }
    *out = *source;
    switch (source->type) {
    case JSON_TREE_STRING:
        out->u.text = copy_text(source->u.text, source->count);
        break;
    case JSON_TREE_BYTES:
        out->u.bytes = allocate(source->count, 1);
        if (source->count)
            memcpy(out->u.bytes, source->u.bytes, source->count);
        break;
    case JSON_TREE_ARRAY:
        out->u.items = allocate(source->count, sizeof(*out->u.items));
        for (i = 0; i < source->count; ++i)
            json_value_copy(&out->u.items[i], &source->u.items[i]);
        break;
    case JSON_TREE_OBJECT:
        out->u.members = allocate(source->count, sizeof(*out->u.members));
        for (i = 0; i < source->count; ++i) {
            const struct JsonTreeMember *m = &source->u.members[i];
            out->u.members[i].key = copy_text(m->key, m->key_length);
            out->u.members[i].key_length = m->key_length;
            json_value_copy(&out->u.members[i].value, &m->value);
        }
        break;
    }
}

void json_value_assign(struct JsonTree *out, const struct JsonTree *source)
{
    struct JsonTree copy;
    json_value_copy(&copy, source);
    json_tree_free(out);
    *out = copy;
}

static struct JsonTree *member(struct JsonTree *object, const char *key, size_t length)
{
    size_t at = 0, n = object->count;
    struct JsonTreeMember *members;
    if (object->type == JSON_TREE_NULL)
        object->type = JSON_TREE_OBJECT;
    if (object->type != JSON_TREE_OBJECT)
        return NULL;
    while (at < n) {
        const struct JsonTreeMember *m = &object->u.members[at];
        int order = memcmp(m->key, key, m->key_length < length ? m->key_length : length);
        if (order == 0)
            order = m->key_length < length ? -1 : m->key_length > length;
        if (order == 0)
            return &object->u.members[at].value;
        if (order > 0)
            break;
        ++at;
    }
    members = allocate(n + 1, sizeof(*members));
    if (at)
        memcpy(members, object->u.members, at * sizeof(*members));
    if (n > at)
        memcpy(members + at + 1, object->u.members + at, (n - at) * sizeof(*members));
    members[at].key = copy_text(key, length);
    members[at].key_length = length;
    free(object->u.members);
    object->u.members = members;
    object->count = n + 1;
    return &members[at].value;
}

struct JsonTree *json_value_member(struct JsonTree *object, const char *key)
{
    return object ? member(object, key, strlen(key)) : NULL;
}

struct JsonTree *json_value_at(struct JsonTree *array, unsigned index)
{
    struct JsonTree *items;
    size_t count = (size_t)index + 1;
    if (!array || !count)
        return NULL;
    if (array->type == JSON_TREE_NULL)
        array->type = JSON_TREE_ARRAY;
    if (array->type != JSON_TREE_ARRAY)
        return NULL;
    if (index >= array->count) {
        items = allocate(count, sizeof(*items));
        if (array->count)
            memcpy(items, array->u.items, array->count * sizeof(*items));
        free(array->u.items);
        array->u.items = items;
        array->count = count;
    }
    return &array->u.items[index];
}

struct JsonTree *json_value_get(const struct JsonTree *object, struct JsonTree *out,
                    const char *key, const struct JsonTree *fallback)
{
    const struct JsonTree *value = object ? json_tree_find(object, key, strlen(key)) : NULL;
    json_value_copy(out, value ? value : fallback);
    return out;
}

void json_value_append(struct JsonTree *array, const struct JsonTree *value)
{
    struct JsonTree copy, *slot;
    json_value_copy(&copy, value);
    if (array->count >= UINT_MAX)
        abort();
    slot = json_value_at(array, (unsigned)array->count);
    if (!slot)
        abort();
    *slot = copy;
}

size_t json_value_size(const struct JsonTree *value)
{
    return value && (value->type == JSON_TREE_ARRAY || value->type == JSON_TREE_OBJECT) ? value->count : 0;
}

struct JsonTree *json_value_child(struct JsonTree *parent, size_t index)
{
    if (index >= json_value_size(parent))
        return NULL;
    return parent->type == JSON_TREE_ARRAY ? &parent->u.items[index] : &parent->u.members[index].value;
}

double json_value_as_double(const struct JsonTree *value)
{
    if (!value)
        return 0;
    switch (value->type) {
    case JSON_TREE_INT64: return (double)value->u.i64;
    case JSON_TREE_UINT64: return (double)value->u.u64;
    case JSON_TREE_DOUBLE: return value->u.dbl;
    case JSON_TREE_BOOL: return value->boolean;
    default: return 0;
    }
}

int json_value_as_int(const struct JsonTree *value)
{
    double number = json_value_as_double(value);
    return number >= INT_MIN && number <= INT_MAX ? (int)number : 0;
}

char json_value_as_bool(const struct JsonTree *value)
{
    if (!value)
        return 0;
    switch (value->type) {
    case JSON_TREE_STRING: return value->count && value->u.text[0];
    case JSON_TREE_ARRAY:
    case JSON_TREE_OBJECT: return value->count != 0;
    default: return json_value_as_double(value) != 0;
    }
}

struct TextView json_value_text(const struct JsonTree *value)
{
    if (value && value->type == JSON_TREE_STRING)
        return text_view(value->u.text, value->count);
    if (value && value->type == JSON_TREE_BOOL)
        return text_view_cstr(value->boolean ? "true" : "false");
    return text_view("", 0);
}

char json_value_is_null(const struct JsonTree *value)
{
    return !value || value->type == JSON_TREE_NULL;
}

char json_value_is_object(const struct JsonTree *value)
{
    return value && value->type == JSON_TREE_OBJECT;
}

char json_value_empty(const struct JsonTree *value)
{
    return json_value_is_null(value) ||
        ((value->type == JSON_TREE_ARRAY || value->type == JSON_TREE_OBJECT) && !value->count);
}

static void read_value(struct JsonTree *out, yyjson_val *value)
{
    size_t index, count;
    yyjson_val *child, *key;
    json_tree_init(out);
    if (yyjson_is_null(value))
        return;
    if (yyjson_is_bool(value)) {
        out->type = JSON_TREE_BOOL;
        out->boolean = yyjson_get_bool(value);
    } else if (yyjson_is_sint(value)) {
        out->type = JSON_TREE_INT64;
        out->u.i64 = yyjson_get_sint(value);
    } else if (yyjson_is_uint(value)) {
        out->type = JSON_TREE_UINT64;
        out->u.u64 = yyjson_get_uint(value);
    } else if (yyjson_is_real(value)) {
        out->type = JSON_TREE_DOUBLE;
        out->u.dbl = yyjson_get_real(value);
    } else if (yyjson_is_str(value)) {
        from_text(out, yyjson_get_str(value), yyjson_get_len(value));
    } else if (yyjson_is_arr(value)) {
        out->type = JSON_TREE_ARRAY;
        out->count = yyjson_arr_size(value);
        out->u.items = allocate(out->count, sizeof(*out->u.items));
        yyjson_arr_foreach(value, index, count, child)
            read_value(&out->u.items[index], child);
    } else if (yyjson_is_obj(value)) {
        out->type = JSON_TREE_OBJECT;
        out->count = yyjson_obj_size(value);
        out->u.members = allocate(out->count, sizeof(*out->u.members));
        yyjson_obj_foreach(value, index, count, key, child) {
            struct JsonTreeMember *m = &out->u.members[count - index - 1];
            m->key_length = yyjson_get_len(key);
            m->key = copy_text(yyjson_get_str(key), m->key_length);
            read_value(&m->value, child);
        }
        if (!json_tree_sort_members(out))
            abort();
    }
}

char json_parse_text(struct JsonTree *root, const char *text, size_t length)
{
    yyjson_doc *doc;
    json_tree_free(root);
    doc = yyjson_read(text, length,
                     YYJSON_READ_ALLOW_COMMENTS | YYJSON_READ_ALLOW_INVALID_UNICODE);
    if (!doc)
        return 0;
    read_value(root, yyjson_doc_get_root(doc));
    yyjson_doc_free(doc);
    return 1;
}

static yyjson_mut_val *write_value(yyjson_mut_doc *doc, const struct JsonTree *value)
{
    yyjson_mut_val *out = NULL;
    size_t i;
    switch (value->type) {
    case JSON_TREE_NULL: return yyjson_mut_null(doc);
    case JSON_TREE_BOOL: return yyjson_mut_bool(doc, value->boolean != 0);
    case JSON_TREE_INT64: return yyjson_mut_sint(doc, value->u.i64);
    case JSON_TREE_UINT64: return yyjson_mut_uint(doc, value->u.u64);
    case JSON_TREE_DOUBLE: return yyjson_mut_real(doc, value->u.dbl);
    case JSON_TREE_STRING: return yyjson_mut_strn(doc, value->u.text, value->count);
    case JSON_TREE_ARRAY:
        out = yyjson_mut_arr(doc);
        if (!out) return NULL;
        for (i = 0; i < value->count; ++i) {
            yyjson_mut_val *child = write_value(doc, &value->u.items[i]);
            if (!child || !yyjson_mut_arr_append(out, child)) return NULL;
        }
        break;
    case JSON_TREE_OBJECT:
        out = yyjson_mut_obj(doc);
        if (!out) return NULL;
        for (i = 0; i < value->count; ++i) {
            const struct JsonTreeMember *member = &value->u.members[i];
            yyjson_mut_val *key = yyjson_mut_strn(doc, member->key, member->key_length);
            yyjson_mut_val *child;
            if (!key) return NULL;
            child = write_value(doc, &member->value);
            if (!child || !yyjson_mut_obj_add(out, key, child)) return NULL;
        }
        break;
    default: break;
    }
    return out;
}

int json_write_request(const struct JsonTree *root, struct OwnedText *out)
{
    yyjson_mut_doc *doc;
    yyjson_mut_val *value;
    char *text;
    size_t size;
    int ok = 0;
    if (!root || !out) return 0;
    doc = yyjson_mut_doc_new(NULL);
    if (!doc) return 0;
    value = write_value(doc, root);
    if (value) {
        yyjson_mut_doc_set_root(doc, value);
        text = yyjson_mut_write(doc, YYJSON_WRITE_PRETTY | YYJSON_WRITE_NEWLINE_AT_END |
                               YYJSON_WRITE_ALLOW_INVALID_UNICODE, &size);
        if (text) {
            ok = owned_text_assign(out, text_view(text, size));
            free(text);
        }
    }
    yyjson_mut_doc_free(doc);
    return ok;
}
