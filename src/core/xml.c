#include "std/string.h"
#include <string.h>
#include "core/xml.h"

struct XmlElement *xml_element_construct(struct XmlElement *self, const char *name)
{
    self->name = name;
    self->start = NULL;
    self->end = NULL;
    string_init(&self->text);
    self->next = NULL;
    self->first = NULL;
    return self;
}

void xml_element_destruct(struct XmlElement *self)
{
    xml_element_clear(self);
    string_free(&self->text);
}

void xml_element_clear(struct XmlElement *self)
{
    struct XmlElement *other;

    other = self->next;
    if (other) {
        xml_element_clear(other);
        string_free(&other->text);
        mem_free(other);
        self->next = NULL;
    }
    other = self->first;
    if (other) {
        xml_element_clear(other);
        string_free(&other->text);
        mem_free(other);
        self->first = NULL;
    }
    self->end = NULL;
    self->start = NULL;
}

void xml_add_child(struct XmlElement *self, struct XmlElement *child)
{
    struct XmlElement *first = self->first;

    if (!first)
        self->first = child;
    else if (!first->next)
        first->next = child;
    else
        xml_add_sibling(first->next, child);
}

void xml_add_sibling(struct XmlElement *self, struct XmlElement *sibling)
{
    while (self->next)
        self = self->next;
    self->next = sibling;
}

struct XmlElement *xml_find_child(struct XmlElement *self, const char *name)
{
    struct XmlElement *first = self->first;

    if (!first)
        return NULL;
    if (strcmp(first->name, name) == 0)
        return first;
    return xml_find_next(first, name);
}

struct XmlElement *xml_find_next(struct XmlElement *self, const char *name)
{
    struct XmlElement *at;

    for (at = self->next; at; at = at->next)
        if (strcmp(at->name, name) == 0)
            return at;
    return NULL;
}

struct String *xml_node_text(struct XmlElement *self)
{
    struct XmlElement *child, *sibling;
    struct String decoded;
    const char *at, *end;
    char c;

    if (self->text.size != 0)
        return &self->text;
    at = self->start;
    while (*at != '>' && *at)
        at++;
    end = self->end;
    at++;
    if (at == end)
        return &self->text;
    do {
        for (child = self->first; child; child = child->first) {
            if (at == child->start)
                at = child->end;
            for (sibling = child->next; sibling; sibling = sibling->next)
                if (at == sibling->start)
                    at = sibling->end;
        }
        c = *at;
        if (c == '<')
            return &self->text;
        if (c != '\r' && c != '\n') {
            if (c == '&') {
                if (strcmp(self->name, "registrationweb") != 0) {
                    xml_entity(self, &decoded, at + 1);
                    string_append(&self->text, string_cstr(&decoded), decoded.size);
                    string_free(&decoded);
                    while (at != end && *at != ';')
                        at++;
                } else {
                    string_append_char(&self->text, '&');
                }
            } else {
                string_append_char(&self->text, c);
            }
        }
        at++;
    } while (at != end);
    return &self->text;
}

struct String *xml_entity(struct XmlElement *self, struct String *out, const char *text)
{
    string_init(out);
    if (text[0] == 'l' && text[1] == 't')
        string_assign_cstr(out, "<");
    else if (text[0] == 'g' && text[1] == 't')
        string_assign_cstr(out, ">");
    else if (text[0] == 'a' && text[1] == 'm' && text[2] == 'p')
        string_assign_cstr(out, "&");
    return out;
}

char *xml_cut_name(struct XmlDocument *doc, char *text, char *end)
{
    char *at = text;

    while (at != end && *at != ' ' && *at != '\t' && *at != '>' && *at != '/')
        at++;
    *at = 0;
    return text;
}

static char *xml_tag_close(char *at, char *end)
{
    while (at != end - 1 && *at != '>')
        at++;
    return at;
}

static struct XmlElement *xml_element_new(struct XmlDocument *doc, char *name, char *end)
{
    struct XmlElement *element = (struct XmlElement *)mem_alloc(sizeof *element);

    if (!element)
        return NULL;
    return xml_element_construct(element, xml_cut_name(doc, name, end));
}

char *xml_parse_children(struct XmlDocument *doc, struct XmlElement *parent,
                                    char *text, char *end)
{
    struct XmlElement *element;
    char *after;

    while (text != end) {
        if (*text == '<') {
            if (text[1] == '/') {
                after = xml_tag_close(text, end) + 1;
                xml_cut_name(doc, text + 2, end);
                if (strcmp(text + 2, parent->name) == 0) {
                    parent->end = after;
                    return after;
                }
                text = after;
                continue;
            }
            if (text[1] == '?' || text[1] == '!') {
                text = xml_tag_close(text, end) + 1;
                continue;
            }
            after = xml_tag_close(text, end) + 1;
            element = xml_element_new(doc, text + 1, end);
            xml_add_child(parent, element);
            if (after[-2] == '/') {
                text = after;
                continue;
            }
            element->start = text;
            text = xml_parse_children(doc, element, after, end);
            continue;
        }
        if (*text == '&' && strcmp(parent->name, "registrationweb") != 0)
            while (text != end && *text != ';')
                text++;
        text++;
    }
    return text;
}

unsigned char xml_parse(struct XmlDocument *doc, const char *text, const char *end)
{
    size_t size;

    if (doc->root.first) {
        xml_element_clear(&doc->root);
        if (doc->copy) {
            mem_free(doc->copy);
            doc->copy = NULL;
        }
    }
    size = (size_t)(end - text);
    doc->copy = (char *)mem_alloc(size + 1);
    memcpy(doc->copy, text, size);
    doc->copy[size] = 0;
    doc->root.start = doc->copy;
    doc->root.end = doc->copy + size;
    xml_parse_children(doc, &doc->root, doc->copy, doc->copy + size);
    return doc->root.first != NULL;
}

void xml_document_release(struct XmlDocument *doc)
{
    xml_element_clear(&doc->root);
    if (doc->copy) {
        mem_free(doc->copy);
        doc->copy = NULL;
    }
}
