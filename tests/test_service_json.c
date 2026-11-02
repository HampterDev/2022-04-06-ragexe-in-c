#include "json/service_json.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


char *native_json_roundtrip(const char *input, size_t length);

static char is_null(const struct JsonTree *value)
{
    return !value || value->type == JSON_TREE_NULL;
}

static double as_double(const struct JsonTree *value)
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

static int as_int(const struct JsonTree *value)
{
    double number = as_double(value);
    return number >= INT_MIN && number <= INT_MAX ? (int)number : 0;
}

static int parse(struct JsonTree *out, const char *text)
{
    return json_parse_text(out, text, strlen(text));
}

static void ownership(void)
{
    struct JsonTree root, item, copy;
    struct OwnedText text = {0}, result = {0};
    struct TextView view;
    const char counted[] = {'a', 0, 'b', '\x80', '\xff'};
    json_tree_init(&root);
    assert(owned_text_assign(&text, text_view(counted, sizeof counted)));
    json_value_from_text(&item, owned_text_view(&text));
    owned_text_clear(&text);
    json_value_append(&root, &item);
    json_tree_free(&item);
    json_value_copy(&copy, &root);
    json_tree_free(&root);
    assert(copy.count == 1);
    view = json_value_text(json_value_at(&copy, 0));
    assert(view.size == sizeof counted);
    assert(!memcmp(view.data, counted, sizeof counted));
    json_value_assign(&copy, &copy);
    assert(copy.count == 1);
    assert(json_write_request(&copy, &result));
    assert(strstr(owned_text_cstr(&result), "\\u0000"));
    assert(json_parse_text(&root, owned_text_cstr(&result), result.size));
    assert(root.u.items[0].count == sizeof counted);
    assert(!memcmp(root.u.items[0].u.text, counted, sizeof counted));
    owned_text_clear(&result);
    json_value_assign(&copy, &copy.u.items[0]);
    assert(copy.type == JSON_TREE_STRING && copy.count == sizeof counted);
    json_tree_free(&copy);
    json_tree_free(&root);
    json_value_copy(&copy, NULL);
    assert(is_null(&copy));
    json_tree_free(&copy);
}

static void fields(void)
{
    struct JsonTree root, number;
    json_tree_init(&root);
    assert(parse(&root, "/* comment */ {\"z\":1,\"a\":2,\"z\":3,\"max\":4294967295,\"min\":-2147483648}"));
    assert(root.count == 4);
    assert(!strcmp(root.u.members[0].key, "a"));
    assert(as_int(json_value_member(&root, "z")) == 3);
    assert(as_int(json_value_member(&root, "min")) == INT_MIN);
    assert((unsigned)as_double(json_value_member(&root, "max")) == UINT_MAX);
    assert(as_int(json_value_member(&root, "max")) == 0);
    assert(is_null(json_value_member(&root, "absent")));
    assert(root.count == 5);
    json_value_from_int(&number, 42);
    json_value_assign(json_value_member(&root, "a"), &number);
    json_tree_free(&number);
    assert(as_int(json_value_member(&root, "a")) == 42);
    json_tree_free(&root);
    json_value_at(&root, 4);
    assert(root.count == 5 && is_null(&root.u.items[3]));
    json_tree_free(&root);
}

static void invalid_input(void)
{
    const char *cases[] = {"", "{", "[1,]", "{\"a\":}", "\"\\q\"", "/*", "true trailing", "1e999", "[NaN]"};
    struct JsonTree root;
    char deeply_nested[601];
    size_t i;
    json_tree_init(&root);
    for (i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        assert(!parse(&root, cases[i]));
        assert(is_null(&root));
    }
    memset(deeply_nested, '[', 300);
    memset(deeply_nested + 300, ']', 300);
    deeply_nested[600] = 0;
    assert(!parse(&root, deeply_nested));
    assert(parse(&root, "null"));
    assert(is_null(&root));
    json_tree_free(&root);
}

static void borrowed_input(void)
{
    static const char text[] = "{\"Type\":1}invalid suffix";
    char counted[] = {'[', '"', 'a', '"', ']'};
    struct JsonTree root;
    json_tree_init(&root);
    assert(json_parse_text(&root, text, 10));
    assert(as_int(json_value_member(&root, "Type")) == 1);
    json_tree_free(&root);
    assert(!json_parse_text(&root, text, sizeof text - 1));
    assert(is_null(&root));
    assert(json_parse_text(&root, counted, sizeof counted));
    counted[2] = 'b';
    assert(root.count == 1 && root.u.items[0].type == JSON_TREE_STRING);
    assert(root.u.items[0].count == 1 && root.u.items[0].u.text[0] == 'a');
    json_tree_free(&root);
}

int main(void)
{
    unsigned i;
    for (i = 0; i < 100; ++i)
        ownership();
    fields();
    invalid_input();
    borrowed_input();
    puts("service JSON: ownership, counted text, numbers, duplicate keys and parse failures passed");
    return 0;
}
