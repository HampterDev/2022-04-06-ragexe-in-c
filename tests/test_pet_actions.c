#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

/* Only external session/world/transport boundaries are replaced. */
struct Session { int petEmotion[5][5][7]; };
struct GameMode { void *world; };
struct GameActor { int token; };
static struct GameActor pet;
static int world_token, pet_present, sent, shown, emotion, action_value;
static unsigned lookups, packet_id;

static struct GameActor *world_get_game_actor_by_aid(void *world, unsigned aid)
{
    assert(world == &world_token && aid == 1234);
    ++lookups;
    return pet_present ? &pet : NULL;
}
static void actor_send(struct GameActor *actor, int msg, long long first, long long second)
{
    assert(actor == &pet && msg == 0x15 && second == 0);
    emotion = (int)first;
    ++shown;
}
static void send_u32(unsigned short id, int value)
{
    packet_id = id;
    action_value = value;
    ++sent;
}

#include "pet_actions.inc"

static void receive_action(int value, int present, int expected_emotion)
{
    struct GameMode mode = { &world_token };
    struct PACKET_ZC_PET_ACT packet = { 0x1aa, 1234, value }, before = packet;
    int previous = shown;
    pet_present = present;
    game_mode_zc_pet_act(&mode, (unsigned char *)&packet);
    assert(!memcmp(&packet, &before, sizeof packet));
    assert(shown == previous + (expected_emotion >= 0));
    if (expected_emotion >= 0)
        assert(emotion == expected_emotion);
}

int main(void)
{
    struct Session session = {0};
    int hunger, intimacy, activity, tested = 0;
    unsigned previous_lookups;
    assert(sizeof(struct PACKET_ZC_PET_ACT) == 10);
    session_init_pet_emotions(&session);
    for (hunger = 0; hunger < 5; ++hunger)
        for (intimacy = 0; intimacy < 5; ++intimacy)
            for (activity = 0; activity < 7; ++activity) {
                int value = session_get_pet_emotion(&session, hunger, intimacy, activity);
                int previous = sent;
                assert(game_mode_msg_pet_act(value) == 0);
                if (value == -1) {
                    assert(sent == previous);
                } else {
                    assert(sent == previous + 1 && packet_id == 0x1a9 && action_value == value);
                    receive_action(value, 1, value / 10);
                    receive_action(value, 0, -1);
                }
                ++tested;
            }
    previous_lookups = lookups;
    /* Speech for both sex encodings, other action kinds and disabled values
       must neither send requests nor reach actor/chat presentation. */
    {
        static const int ignored[] = {INT_MIN, -1, 0, 1, 3, 9, 1002000, 1002001, INT_MAX};
        unsigned i;
        for (i = 0; i < sizeof ignored / sizeof *ignored; ++i) {
            int previous = sent;
            assert(game_mode_msg_pet_act(ignored[i]) == 0 && sent == previous);
            receive_action(ignored[i], 1, -1);
        }
    }
    assert(lookups == previous_lookups);
    printf("Pet actions: %d emotion-table combinations, missing actors, ignored speech and unchanged packets passed.\n", tested);
    return 0;
}
