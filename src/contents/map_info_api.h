#ifndef CONTENTS_MAP_INFO_API_H
#define CONTENTS_MAP_INFO_API_H

struct MapInfoMgr;
extern struct MapInfoMgr *g_mapInfo;

char map_info_create(void);
void map_info_destroy(void);
char map_info_load(struct MapInfoMgr *self);
int map_info_set_display_name(struct MapInfoMgr *self, const char *map,
                              const char *name, char show_banner);
int map_info_set_sign(struct MapInfoMgr *self, const char *map,
                      const char *line1, const char *line2);
int map_info_set_ornament(struct MapInfoMgr *self, const char *map,
                          const char *ornament);
const char *map_info_display_name(const struct MapInfoMgr *self, const char *map);
void map_info_show_banner(struct MapInfoMgr *self, const char *map);
void map_info_render_banner(struct MapInfoMgr *self);

#endif
