#include "core/xml.h"
#include "std/string.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

__declspec(noreturn) void fatal_error(const char *message)
{
    puts(message);
    exit(1);
}

static const char *text_of(struct XmlElement *parent, const char *name)
{
    struct XmlElement *child = xml_find_child(parent, name);

    assert(child);
    return string_cstr(xml_node_text(child));
}

static void read_clientinfo(const char *empty_tag)
{
    char source[512];
    struct XmlDocument doc;
    struct XmlElement *info, *connection;
    int length = snprintf(source, sizeof source,
        "<?xml version=\"1.0\" encoding=\"euc-kr\" ?>\n<clientinfo>\n"
        "\t<servicetype>korea</servicetype>\n\t%s\n\t<connection>\n"
        "\t\t<address>127.0.0.1</address>\n\t\t<port>6900</port>\n"
        "\t\t<aid><admin>2000000</admin></aid>\n\t</connection>\n</clientinfo>\n", empty_tag);

    assert(length > 0 && (size_t)length < sizeof source);
    xml_element_construct(&doc.root, "XMLDocument");
    doc.copy = NULL;
    assert(xml_parse(&doc, source, source + length));
    info = xml_find_child(&doc.root, "clientinfo");
    assert(info && xml_find_child(info, "readfolder"));
    assert(!strcmp(text_of(info, "servicetype"), "korea"));
    connection = xml_find_child(info, "connection");
    assert(connection);
    assert(!strcmp(text_of(connection, "address"), "127.0.0.1"));
    assert(!strcmp(text_of(connection, "port"), "6900"));
    assert(!strcmp(text_of(xml_find_child(connection, "aid"), "admin"), "2000000"));
    xml_document_release(&doc);
    xml_element_destruct(&doc.root);
}

static void read_ignored_url(const char *tag)
{
    char source[512];
    struct XmlDocument doc;
    struct XmlElement *info, *connection;
    int length = snprintf(source, sizeof source,
        "<clientinfo><connection><%s>https://example.invalid/?a=1&b=2</%s>"
        "<address>127.0.0.1</address><description>One &amp; Two</description>"
        "<port>6900</port></connection></clientinfo>", tag, tag);

    assert(length > 0 && (size_t)length < sizeof source);
    xml_element_construct(&doc.root, "XMLDocument");
    doc.copy = NULL;
    assert(xml_parse(&doc, source, source + length));
    info = xml_find_child(&doc.root, "clientinfo");
    assert(info);
    connection = xml_find_child(info, "connection");
    assert(connection && xml_find_child(connection, tag));
    assert(!strcmp(text_of(connection, "address"), "127.0.0.1"));
    assert(!strcmp(text_of(connection, "description"), "One & Two"));
    assert(!strcmp(text_of(connection, "port"), "6900"));
    xml_document_release(&doc);
    xml_element_destruct(&doc.root);
}

int main(void)
{
    read_clientinfo("<readfolder/>");
    read_clientinfo("<readfolder />");
    read_clientinfo("<readfolder></readfolder>");
    read_ignored_url("registrationweb");
    read_ignored_url("unused");
    puts("XML: empty/nested elements, ignored URLs and entity-decoded text passed");
    return 0;
}
