#include "session/csession.h"
#include "session/session_skills.h"

#define BIT_SET(state, n)          ((BYTE)(((DWORD)(state) >> (n)) & 1))
#define BIT_CHANGED(a, b, n)       ((BYTE)((((DWORD)(a) >> (n)) ^ ((DWORD)(b) >> (n))) & 1))

#define EFFECT_STATE_BIT(n) \
    BYTE effect_state_bit##n(DWORD state) { return BIT_SET(state, n); }
#define EFFECT_STATE_BIT_CHANGED(n) \
    BYTE effect_state_bit##n##_changed(DWORD a, DWORD b) { return BIT_CHANGED(a, b, n); }

BYTE effect_state_bit0(DWORD state) { return (BYTE)(state & 1); }
BYTE effect_state_bit0_changed(DWORD a, DWORD b) { return (BYTE)((b ^ a) & 1); }

BYTE is_effect_state_burrow(DWORD state) { return BIT_SET(state, 1); }
EFFECT_STATE_BIT_CHANGED(1)
BYTE is_effect_state_star2(DWORD state) { return BIT_SET(state, 2); }
EFFECT_STATE_BIT_CHANGED(2)
EFFECT_STATE_BIT(4)  EFFECT_STATE_BIT_CHANGED(4)
EFFECT_STATE_BIT(5)  EFFECT_STATE_BIT_CHANGED(5)
EFFECT_STATE_BIT(6)  EFFECT_STATE_BIT_CHANGED(6)
EFFECT_STATE_BIT(11)
EFFECT_STATE_BIT(12)
EFFECT_STATE_BIT(13) EFFECT_STATE_BIT_CHANGED(13)
EFFECT_STATE_BIT(14)
EFFECT_STATE_BIT(15) EFFECT_STATE_BIT_CHANGED(15)
EFFECT_STATE_BIT(16)

EFFECT_STATE_BIT(18)
EFFECT_STATE_BIT(19) EFFECT_STATE_BIT_CHANGED(19)
EFFECT_STATE_BIT(20) EFFECT_STATE_BIT_CHANGED(20)
EFFECT_STATE_BIT(21) EFFECT_STATE_BIT_CHANGED(21)
EFFECT_STATE_BIT(22)
EFFECT_STATE_BIT(23) EFFECT_STATE_BIT_CHANGED(23)
EFFECT_STATE_BIT(24) EFFECT_STATE_BIT_CHANGED(24)
EFFECT_STATE_BIT(25) EFFECT_STATE_BIT_CHANGED(25)
EFFECT_STATE_BIT(26) EFFECT_STATE_BIT_CHANGED(26)
EFFECT_STATE_BIT(27)
EFFECT_STATE_BIT(28)

BYTE effect_state_none(DWORD state) { return state == 0; }

void effect_state_set_bit6(DWORD *state) { *state |= 0x40; }
void effect_state_clear_bit6(DWORD *state) { *state &= 0xffffffbf; }
