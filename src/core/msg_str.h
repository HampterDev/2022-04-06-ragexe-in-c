#ifndef CORE_MSG_STR_H
#define CORE_MSG_STR_H

#define MSG_STRING_COUNT 3985

void msg_strings_init(void);
void msg_str_load(void);
void msg_str_unload(void);
char *msg_str(int id);
const char *msg_str_to_compare(int id);
const char *msg_str_field(unsigned id);

#endif
