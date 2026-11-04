#ifndef CORE_XML_TYPES_H
#define CORE_XML_TYPES_H

#include "std/string.h"

struct XmlElement {
    const char *name;
    const char *start;
    const char *end;
    struct String text;
    struct XmlElement *next;
    struct XmlElement *first;
};

struct XmlDocument {
    struct XmlElement root;
    char *copy;
};

#endif
