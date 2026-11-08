#include "game/client_state.h"

/* Defined with their header, below their readers and writers: the map property packet
   handler (game/map_property.c), the whisper code (game/mode_whisper.c, game/msg_chat.c)
   and the session's friend greeting (session/session_party.c). */
unsigned char g_mapPropertyQuiet;
signed char g_silentWhisperAcks;
