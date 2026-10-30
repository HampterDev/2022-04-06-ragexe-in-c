#ifndef CORE_XML_TYPES_H
#define CORE_XML_TYPES_H

#include "base.h"

struct XmlElement {
    const char *name;
    const char *start;
    const char *end;
    struct String text;
    struct XmlElement *next;
    struct XmlElement *first;
};
OFFSET_ASSERT(XmlElement, text, 0x0c);

struct XmlDocument {
    struct XmlElement root;
    char *copy;
};

#endif
