#ifndef LOGINMODE_LIFECYCLE_H
#define LOGINMODE_LIFECYCLE_H

struct Mode;

struct Mode *login_mode_create(void);
void login_mode_destroy(struct Mode *mode);
void login_mode_on_init(struct Mode *mode, const char *name);
int login_mode_on_run(struct Mode *mode);
void login_mode_on_exit(struct Mode *mode);

#endif
