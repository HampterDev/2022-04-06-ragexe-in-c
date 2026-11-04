#ifndef CORE_XML_H
#define CORE_XML_H

#include "core/xml_types.h"

struct XmlElement *xml_element_construct(struct XmlElement *self, const char *name);
void xml_element_destruct(struct XmlElement *self);
void xml_element_clear(struct XmlElement *self);
void xml_add_child(struct XmlElement *self, struct XmlElement *child);
void xml_add_sibling(struct XmlElement *self, struct XmlElement *sibling);
struct XmlElement *xml_find_child(struct XmlElement *self, const char *name);
struct XmlElement *xml_find_next(struct XmlElement *self, const char *name);
struct String *xml_node_text(struct XmlElement *self);
struct String *xml_entity(struct XmlElement *self, struct String *out, const char *text);
char *xml_cut_name(struct XmlDocument *doc, char *text, char *end);
char *xml_parse_children(struct XmlDocument *doc, struct XmlElement *parent,
                                    char *text, char *end);
unsigned char xml_parse(struct XmlDocument *doc, const char *text, const char *end);
void xml_document_release(struct XmlDocument *doc);

#endif
