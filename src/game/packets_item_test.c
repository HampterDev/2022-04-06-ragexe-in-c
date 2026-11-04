#include "game/packet_handlers.h"
#include "platform/win_types.h"
#include <stdio.h>
#include "core/client_path.h"
#include "platform/filesystem.h"
#include "game/gamemode.h"
#include "game/item_info.h"
#include "ui/items/styling_shop.h"
#include "ui/ui_std.h"
#include "core/app.h"
#include "platform/clock.h"

#pragma pack(push, 1)
struct PackageItemTestEntry {
    int item_id;
    unsigned short amount;
    int count;
};
struct PACKET_ZC_RESULT_PACKAGE_ITEM_TEST {
    unsigned short packet_type;
    short packet_length;
    int package_item_id;
    struct PackageItemTestEntry records[];
};
#pragma pack(pop)
SIZE_ASSERT(PackageItemTestEntry, 10);
OFFSET_ASSERT(PACKET_ZC_RESULT_PACKAGE_ITEM_TEST, records, 8);

void game_mode_zc_result_package_item_test(struct GameMode *self, const unsigned char *pkt)
{
    const struct PACKET_ZC_RESULT_PACKAGE_ITEM_TEST *packet = (const struct PACKET_ZC_RESULT_PACKAGE_ITEM_TEST *)pkt;
    const char *package_name;
    char path[0x800];
    struct ItemInfo item;
    struct PlatformDateTime now;
    FILE *f;
    const struct PackageItemTestEntry *entry = packet->records;
    int n = 0, records, total, written;

    (void)self;

    item_info_construct(&item);
    item_info_set_item_id(&item, packet->package_item_id);
    package_name = item_info_get_name(&item);
    if (!client_path(path, sizeof path, "PackageItemTest")) goto done;
    if (!platform_path_exists(path) && platform_directory_create(path))
        goto done;
    platform_date_time_local(&now);
    if (!client_path(path, sizeof path, "PackageItemTest\\%s(%d_%d_%d).txt", package_name, now.year, now.month,
                     now.day))
        goto done;
    if (platform_path_exists(path)) {
        do {
            n++;
            if (!client_path(path, sizeof path, "PackageItemTest\\%s(%d_%d_%d)_%d.txt", package_name, now.year,
                             now.month, now.day, n))
                goto done;
        } while (platform_path_exists(path));
    }
    f = fopen(path, "w");
    if (!f)
        goto done;
    records = (int)((unsigned)(packet->packet_length - 8) / 10);
    total = 0;
    fprintf(f, "============================START============================\n");
    for (; records > 0; records--, entry++) {
        const char *name;
        struct PackageItemTestEntry e = *entry;

        item_info_set_item_id(&item, e.item_id);
        name = item_info_get_name(&item);
        fprintf(f, "%s\t\t\t%d\t%c%d%c\n", name, e.count, '(', e.amount, ')');
        total += e.count;
    }
    fprintf(f, "============================END============================\n");
    fprintf(f, "Unpacked %d Packages\n", total);
    fclose(f);
done:
    item_info_destruct(&item);
}
