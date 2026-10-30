#ifndef SESSION_ABR_H
#define SESSION_ABR_H

struct Session;
void session_erase_abr(struct Session *self, long long key);
long long session_find_abr_hp(struct Session *self, long long key);
long long session_find_abr(struct Session *self, long long key);

#endif
