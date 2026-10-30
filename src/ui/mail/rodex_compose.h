#ifndef UI_MAIL_RODEX_COMPOSE_H
#define UI_MAIL_RODEX_COMPOSE_H

struct UIWindow;
struct UIEditCtrl3;

struct UIEditCtrl3 *ui_rodex_compose_body(struct UIWindow *compose);

struct UIRodexComposeWnd;
void rodex_compose_wnd_set_receiver(struct UIRodexComposeWnd *self, const char *name);
struct UIRodexComposeWnd *ui_rodex_compose_wnd_construct(struct UIRodexComposeWnd *self);
struct UIWindow *ui_rodex_compose_wnd_new(void);
void ui_rodex_compose_wnd_set_receiver(struct UIRodexComposeWnd *self, int char_id, WORD job, WORD level, const char *name);

#endif
