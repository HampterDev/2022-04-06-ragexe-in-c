#ifndef GAMEMODE_MACRO_DETECTOR_H
#define GAMEMODE_MACRO_DETECTOR_H

struct GameMode;

void game_mode_on_preview_macro_detector_captcha(struct GameMode *self, const char *key, const void *image,
                                                       unsigned short image_size);
void game_mode_on_req_answer_macro_detector(struct GameMode *self, int retry_count, int timeout);
void game_mode_on_req_au_bot(struct GameMode *self, int aid);

#endif
