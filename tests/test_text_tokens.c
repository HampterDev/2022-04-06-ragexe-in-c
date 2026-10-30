#include "core/text_util.h"
#include "core/fatal.h"
#include "ui/controls/buttons.h"
#include "std/string_vec.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned allocations;

void *std_allocate_bytes(size_t bytes)
{
    void *block = malloc(bytes);

    assert(block);
    ++allocations;
    return block;
}

void *mem_alloc(size_t bytes)
{
    return std_allocate_bytes(bytes);
}

void mem_free(void *block)
{
    assert(allocations);
    --allocations;
    free(block);
}

void fatal_error(const char *message)
{
    (void)message;
    abort();
}

static size_t split(struct Vec *v, struct String *input, const char *text, size_t bytes)
{
    string_assign(input, text, bytes);
    tokens_split(v, input);
    return v->first ? (struct String *)v->last - (struct String *)v->first : 0;
}

int main(void)
{
    struct Vec words = {0};
    struct String input;
    struct UISpriteButton button = {0};
    char long_text[5000];
    const char *text = " alpha\t beta\r\ngamma ";
    string_init(&input);
    assert(split(&words, &input, text, strlen(text)) == 3);
    assert(strcmp(string_cstr(&((struct String *)words.first)[1]), "beta") == 0);
    text = "first ignored//comment rest";
    assert(split(&words, &input, text, strlen(text)) == 1);
    assert(strcmp(string_cstr((struct String *)words.first), "first") == 0);
    memset(long_text, 'x', sizeof long_text);
    assert(split(&words, &input, long_text, sizeof long_text) == 1);
    assert(((struct String *)words.first)->size == sizeof long_text);
    text = "한글 test";
    assert(split(&words, &input, text, strlen(text)) == 2);
    assert(strcmp(string_cstr((struct String *)words.first), "한글") == 0);
    assert(split(&words, &input, "/", 1) == 1);
    assert(split(&words, &input, " \n\r", 3) == 0);
    string_vec_free(&words);
    string_free(&input);

    string_init(&button.spr);
    string_init(&button.act);
    long_text[sizeof long_text - 1] = 0;
    ui_sprite_button_set_sprite(&button.button.base, long_text);
    assert(button.spr.size == sizeof long_text - 1 && button.act.size == button.spr.size);
    assert(strcmp(string_cstr(&button.spr) + button.spr.size - 3, "spr") == 0);
    assert(strcmp(string_cstr(&button.act) + button.act.size - 3, "act") == 0);
    ui_sprite_button_set_sprite(&button.button.base, string_cstr(&button.spr));
    assert(button.spr.size == sizeof long_text - 1 && button.act.size == button.spr.size);
    ui_sprite_button_set_sprite(&button.button.base, "a");
    assert(button.spr.size == sizeof long_text - 1);
    ui_sprite_button_set_sprite(&button.button.base, "abc");
    assert(strcmp(string_cstr(&button.spr), "spr") == 0);
    assert(strcmp(string_cstr(&button.act), "act") == 0);
    string_free(&button.spr);
    string_free(&button.act);
    assert(!allocations);
    puts("PASS: token spans, comments, CP949 text, long resource paths and aliased path ownership");
    return 0;
}
