#include "ui_string_test.h"
#include "../src/ui/controls/ui_string.c"

static void set_text(struct UIString *word, struct TextView text)
{
#ifdef UI_STRING_REFERENCE
    struct String copy;
    string_init(&copy);
    string_assign(&copy, text.data, text.size);
    ui_string_set_text(word, 100, 100, copy);
#else
    ui_string_set_text(word, 100, 100, text);
#endif
}

static void parse_text(struct UIString *word, struct TextView text)
{
#ifdef UI_STRING_REFERENCE
    struct String copy;
    string_init(&copy);
    string_assign(&copy, text.data, text.size);
    ui_string_parse(word, copy);
#else
    ui_string_parse(word, text);
#endif
}

static void record_word(const struct UIString *word)
{
    test_ui_string_record(&word->text);
    test_ui_string_record(&word->image);
    test_ui_string_record(&word->link);
    test_ui_string_record(&word->font);
    test_ui_string_number(word->r);
    test_ui_string_number(word->g);
    test_ui_string_number(word->b);
    test_ui_string_number(word->size);
    test_ui_string_number(word->base.width);
    test_ui_string_number(word->base.height);
}

void test_ui_string_owner(void)
{
    static const char *const cases[] = {
        "", "short", " a fairly long string beyond inline storage ",
        "\t  앞쪽 공백과 한글  ", "\n", " \n", "\r",
        "<image=\"501\">first attribute behavior<\\end>",
        "<=image=\"501\" color= ff0000 >item<\\end>",
        "<=link=\"poring\" font=\"Gulim\" size= 12 >monster<\\end>",
        "<=color= 123456 link=\"poring\" >link wins<\\end>",
        "<=link=\"poring\" color= 123456 >colour wins<\\end>"
    };
    unsigned i;
    struct UIString word;
    char bytes[] = {'a', '\0', 'b', 'c'};
    char before[sizeof bytes];
    for (i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        memset(&word, 0, sizeof word);
        ui_string_construct(&word);
        set_text(&word, text_view_cstr(cases[i]));
        record_word(&word);
        if (i == 1) assert(word.text.size == 5 && word.base.width == 15 && word.base.height == 12);
        if (i == 4) assert(word.base.width == 5 && word.base.height == 5);
        if (i == 7) assert(!word.image.size);
        if (i == 8) assert(!strcmp(string_cstr(&word.image), "501") && word.r == 255 && !word.g && !word.b);
        if (i == 9) assert(!strcmp(string_cstr(&word.link), "poring") && word.size == 12);
        if (word.text.size > 1) {
            parse_text(&word, text_view(string_cstr(&word.text) + 1, word.text.size - 1));
            record_word(&word);
        }
        set_text(&word, text_view_cstr("replacement"));
        record_word(&word);
        ui_string_destruct(&word);
        test_ui_string_no_allocations();
    }
    memset(&word, 0, sizeof word);
    ui_string_construct(&word);
    memcpy(before, bytes, sizeof bytes);
    set_text(&word, text_view(bytes, sizeof bytes));
    assert(word.text.size == sizeof bytes && !memcmp(string_cstr(&word.text), bytes, sizeof bytes));
    assert(!memcmp(bytes, before, sizeof bytes));
    record_word(&word);
    ui_string_destruct(&word);
    test_ui_string_no_allocations();
}
