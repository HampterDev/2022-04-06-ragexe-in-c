#ifndef CONTENTS_MAP_INFO_H
#define CONTENTS_MAP_INFO_H

struct Texture;
struct DcBitmap;
struct MapNameDisplay {
    struct Texture *ornament;
    struct DcBitmap *band;
    unsigned short band_width;
    unsigned short band_height;
    unsigned start_time;
};

struct MapNameDisplay *map_name_display_construct(struct MapNameDisplay *self,
                                                  const char *line1, const char *line2,
                                                  const char *ornament);
void map_name_display_destruct(struct MapNameDisplay *self);
char map_name_display_render(struct MapNameDisplay *self);

#endif
