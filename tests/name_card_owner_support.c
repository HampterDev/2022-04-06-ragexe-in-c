#include <stdlib.h>
#include "web/name_cards.h"
void *allocate(size_t size);
void *resize(void *p, size_t size);
void release(void *p);
#define malloc allocate
#define realloc resize
#define free release
#include "../src/web/name_cards.c"
