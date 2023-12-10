#ifndef _DAE_IMPORT_H_
#define _DAE_IMPORT_H_
#include "stdafx.h"
#include "win32_application.h"
#include "mesh_common.h"
#include <rapidxml.hpp>

using namespace rapidxml;

#define PRINT_TABS(num) for (auto i = 0; i != num; ++i) printf("\t");

//////////////////////////////
struct DAE_Scene
{
    Mesh_Object *meshes;
};

//////////////////////////////
void print_dae_node(xml_node<> *node, int num_tabs = 0) {

    PRINT_TABS(num_tabs);
    printf("<%s", node->name());

    for (xml_attribute<> *attrib = node->first_attribute(); attrib != NULL; attrib = attrib->next_attribute()) {
        printf(" %s='%s'", attrib->name(), attrib->value());
    }

    xml_node<> *child = node->first_node();
    if (!child) {
        printf("/>\n");
    }
    else {
        if (node->value_size() > 0) {
            printf("></%s>\n", node->name());
            //printf(">%s</%s>\n", node->value(), node->name());
        }
        else {
            printf(">\n");
            for (; child != NULL; child = child->next_sibling()) {
                print_dae_node(child, num_tabs + 1);
            }
            PRINT_TABS(num_tabs);
            printf("</%s>\n", node->name());
        }
    }
}

xml_node<> *find_dae_node(const char *name, xml_node<> *root) {
    if (!strcmp(name, root->name()))
        return root;

    for (auto child = root->first_node(); child != NULL; child = child->next_sibling()) {
        auto result = find_dae_node(name, child);
        if (!result)
            return NULL;
    }

    return NULL;
}

xml_attribute<> *find_dae_attribute_in_node(const char *name, xml_node<> *node) {
    for (auto attrib = node->first_attribute(); attrib != NULL; attrib = attrib->next_sibling()) {
        if (!strcmp(name, attrib->name()))
            return attrib;
    }
    return NULL;
}

xml_node<> *find_dae_node_by_id(const char *id, xml_node<> *root) {
    auto id_attrib = find_dae_attribute_in_node("id", root);
    if (id_attrib && !strcmp(id, id_attrib->value()))
        return root;

    for (auto child = root->first_node(); child != NULL; child = child->next_sibling()) {
        id_attrib = find_dae_node_by_id(id, child);
        if (id_attrib)
            return child;
    }

    return NULL;
}

#define DAE_POSITIONS_ARRAY_NAME "positions-array"
#define DAE_NORMALS_ARRAY_NAME "normals-array"
#define DAE_TANGENTS_ARRAY_NAME "tangents-array"
#define DAE_BITANGENTS_ARRAY_NAME "bitangents-array"
#define DAE_TEX0_ARRAY_NAME "tex0-array"
#define DAE_COLOR0_ARRAY_NAME "color0-array"

xml_node<> *find_dae_array_node_by_name(const char *array_name, xml_node<> *root, char *search_string) {
}

bool import_dae(char *path) {
    int64_t clock_start = get_clock(); // Function timing.
    
    LARGE_INTEGER li;
    char *buffer = NULL;
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 128, NULL);
    ASSERT(file != (HANDLE)-1);
    GetFileSizeEx(file, &li);
    buffer = (char *)malloc(li.QuadPart + 1);
    buffer[li.QuadPart] = 0;
    ReadFile(file, buffer, li.LowPart, NULL, NULL);
    CloseHandle(file);

    xml_document<> doc;
    doc.parse<0>(buffer);

    for (xml_node<> *node = doc.first_node(); node != NULL; node = node->next_sibling())
        print_dae_node(node);
    
    free(buffer);
    printf("Finished: %f\n", (((float)(get_clock() - clock_start)) / (float)win32.clock_freq));
    return true;
}

#endif 
