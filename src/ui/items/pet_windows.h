#ifndef UI_ITEMS_PET_WINDOWS_H
#define UI_ITEMS_PET_WINDOWS_H

struct UIWindow;

void ui_pet_info_update_intimacy(struct UIWindow *wnd);
void ui_pet_info_set_name(struct UIWindow *wnd, const char *name);

struct PetEggDetails {
    short level;
    short hunger;
    short intimacy;
    short accessory;
    int renamed;
    const char *name;
};
void ui_item_collection_set_pet(struct UIWindow *wnd, const struct PetEggDetails *pet);

#endif
