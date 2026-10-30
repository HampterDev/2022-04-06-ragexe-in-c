#ifndef YEXCEL_STATE_ICON_SHEET_H
#define YEXCEL_STATE_ICON_SHEET_H
struct StateIconSheet;
struct CooldownTimer;
extern struct StateIconSheet *g_stateIconSheet;

unsigned char icon_sheet_create(void);
void icon_sheet_destroy(void);
void icon_sheet_set_timer(struct StateIconSheet *sheet, unsigned state, unsigned total_ms,
                          unsigned start, unsigned elapsed);
void icon_sheet_remove_timer(struct StateIconSheet *sheet, unsigned state);
void icon_sheet_queue_state(struct StateIconSheet *sheet, unsigned state, int x, int y);
void icon_sheet_queue(struct StateIconSheet *sheet, int x, int y, int w, int h,
                      struct CooldownTimer *timer, int limit, int dx, int dy);
void icon_sheet_clear(struct StateIconSheet *sheet);
void icon_sheet_flush(struct StateIconSheet *sheet);
void icon_sheet_flush_bars(struct StateIconSheet *sheet);
#endif
