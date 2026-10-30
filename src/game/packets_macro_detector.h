#ifndef GAMEMODE_PACKETS_MACRO_DETECTOR_H
#define GAMEMODE_PACKETS_MACRO_DETECTOR_H

struct GameMode;

void game_mode_zc_ack_preview_macro_detector(struct GameMode *self, int result, int image_size,
                                                       const char *key);
void game_mode_zc_ack_upload_macro_detector(struct GameMode *self, const char *key, int result);
void game_mode_zc_apply_macro_detector(struct GameMode *self, int image_size, const char *key);
void game_mode_zc_apply_macro_detector_captcha(struct GameMode *self, const char *key, const void *data,
                                                         int size);
void game_mode_zc_ack_apply_macro_detector(struct GameMode *self, int result);
void game_mode_zc_close_macro_detector(struct GameMode *self, int result);
void game_mode_zc_complete_upload_macro_detector_captcha(struct GameMode *self);

#endif
