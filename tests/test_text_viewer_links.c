#include "../src/ui/controls/window_links.c"
#include "../src/ui/controls/text_viewer_links.c"
#include <assert.h>
#include <stdio.h>

static struct String text_of(const char *s)
{
    struct String out;
    string_init(&out);
    string_assign_cstr(&out, s);
    return out;
}

int main(void)
{
    struct Vec links = { 0 };
    struct UiTagLink *link;

    ui_window_parse_tag_links(NULL, text_of("Use <ITEM>Red Potion<INFO>501</INFO></ITEM> or <ITEM>Apple<INFO>512</INFO></ITEM>."),
                              text_of("<ITEM>"), text_of("</ITEM>"), &links);
    assert(VEC_COUNT(&links, struct UiTagLink) == 2);
    link = links.first;
    assert(link[0].pos == 4 && !strcmp(string_cstr(&link[0].label), "Red Potion") && !strcmp(string_cstr(&link[0].args), "501"));
    assert(link[1].pos == 18 && !strcmp(string_cstr(&link[1].label), "Apple") && !strcmp(string_cstr(&link[1].args), "512"));
    tag_links_free(&links);
    assert(!links.first);
    puts("Text viewer links: the viewer walks and frees the parser's link records passed.");
    return 0;
}
