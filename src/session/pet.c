#include "contents/client_lua.h"
#include "session/csession.h"
#include "session/session_talk.h"

#include <string.h>

static int pet_level(int value)
{
    if ((unsigned)(value - 91) <= 9)
        return 4;
    if ((unsigned)(value - 76) <= 14)
        return 3;
    if ((unsigned)(value - 26) <= 49)
        return 2;
    return (unsigned)(value - 11) <= 14;
}

int session_pet_hunger_level(int fullness)
{
    return pet_level(fullness);
}

int session_pet_intimacy_level(int intimacy)
{
    int value = 0;

    if (intimacy)
        value = intimacy / 10;
    return pet_level(value);
}

int session_get_pet_emotion(struct Session *self, int hungerLevel, int intimacyLevel,
                                     int action)
{
    int emotion = self->petEmotion[hungerLevel][intimacyLevel][action];

    if (emotion == -1)
        return -1;
    return emotion * 10 + 2;
}

int session_get_sex(struct Session *self)
{
    if (self->sexCopy == self->sex)
        return self->sexCopy;
    self->sexCopy = self->sex;
    return self->sex;
}

void session_set_sex(struct Session *self, int sex)
{
    self->sexCopy = sex & 1;
    self->sex = sex & 1;
}

void session_get_pet_intimacy_text(int intimacy, char *out)
{
    if (intimacy >= 0 && intimacy <= 100)
        strcpy(out, "서먹서먹함");
    else if (intimacy > 100 && intimacy <= 250)
        strcpy(out, "어색함");
    else if (intimacy > 250 && intimacy <= 750)
        strcpy(out, "보통");
    else if (intimacy > 750 && intimacy <= 900)
        strcpy(out, "친함");
    else if (intimacy > 900 && intimacy <= 1000)
        strcpy(out, "절친함");
    else
        strcpy(out, "저는 모릅니다아아아.");
}

void session_get_pet_hunger_text(int fullness, char *out)
{
    if (fullness >= 0 && fullness <= 10)
        strcpy(out, "배고픔");
    else if (fullness > 10 && fullness <= 25)
        strcpy(out, "출출함");
    else if (fullness > 25 && fullness <= 75)
        strcpy(out, "보통");
    else if (fullness > 75 && fullness <= 90)
        strcpy(out, "배부름");
    else if (fullness > 90 && fullness <= 100)
        strcpy(out, "아주 배부름");
    else
        strcpy(out, "몰라요.");
}

void session_get_pet_illust_name(struct Session *self, int petJob, char *out, size_t outSize)
{
    static const char fallback[] = "UserInterface\\illust\\PET_NOIMAGE.bmp";
    const char *name = NULL;
    size_t length;

    if (outSize == 0)
        return;
    if (!client_lua_call(self->luaState, "GetPetIllustName_V3", "db>s", petJob, 1, &name)
        || name == NULL)
        name = fallback;
    length = strlen(name);
    if (length >= outSize) {
        name = fallback;
        length = sizeof fallback - 1;
    }
    if (length >= outSize) {
        out[0] = '\0';
        return;
    }
    memcpy(out, name, length + 1);
}

struct PetAccessoryLookup {
    int accessory;
    int mapped;
};

static int lookup_pet_accessory(lua_State *state)
{
    struct PetAccessoryLookup *lookup = lua_touserdata(state, 1);

    lua_getglobal(state, "PetAccActNameTable");
    if (!lua_istable(state, -1))
        return 0;
    lua_rawgeti(state, -1, lookup->accessory);
    lookup->mapped = lua_type(state, -1) == LUA_TSTRING && lua_objlen(state, -1) != 0;
    return 0;
}

const char *session_get_pet_accessory_act(struct Session *self, int accessory)
{
    struct PetAccessoryLookup lookup = {accessory, 0};
    const char *name = NULL;
    lua_State *state;
    int base, ok;

    if (!self->luaState || !self->luaState->L)
        return NULL;
    state = self->luaState->L;
    base = lua_gettop(state);
    /* The legacy Lua helper substitutes another pet's animation for absent IDs. */
    ok = lua_cpcall(state, lookup_pet_accessory, &lookup) == 0;
    lua_settop(state, base);
    if (ok && lookup.mapped
        && client_lua_call(self->luaState, "GetPetAccActName", "d>s", accessory, &name)
        && name && name[0] && res_mgr_is_available(g_resMgr(), name))
        return name;
    return NULL;
}
