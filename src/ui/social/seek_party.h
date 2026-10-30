#ifndef UI_SOCIAL_SEEK_PARTY_H
#define UI_SOCIAL_SEEK_PARTY_H

#include "ui/social/social.h"
#include "contents/party_booking.h"

void seek_party_load_regions(struct UIWindow *region_combo, struct PartySearchRegions *regions, int first);
int seek_party_show_maps(struct UIWindow *combo, const struct PartySearchRegions *regions, int key);

struct UISeekPartyWnd *ui_seek_party_wnd_construct(struct UISeekPartyWnd *self);
struct UIWindow *ui_seek_party_wnd_new(void);

struct UISeekPartyListWnd *ui_seek_party_list_wnd_construct(struct UISeekPartyListWnd *self);
struct UIWindow *ui_seek_party_list_wnd_new(void);

#endif
