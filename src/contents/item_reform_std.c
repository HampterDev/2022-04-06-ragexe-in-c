#include "contents/item_reform.h"
#include "std/std.h"
#include "std/tree.h"

void reform_info_init(struct ReformInfo *info)
{
    tree_init(&info->materials);
    string_init(&info->random_option_code);
    string_init(&info->information);
}

void reform_info_destroy(struct ReformInfo *info)
{
    string_free(&info->information);
    string_free(&info->random_option_code);
    tree_free(&info->materials, NULL);
}
