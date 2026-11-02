#include "contents/sound.h"
#include "session/state.h"
#include "session/csession.h"
#include "session/session_skills.h"

static void begin_wave(const char *wave)
{
    struct Vec3 origin = { 0.0f, 0.0f, 0.0f };
    sound_play_wave(wave, origin.x, origin.y, origin.z, 0xfa, 0x28, 1.0f);
}

static void effect_by_level(int level, int *effect)
{
    switch (level) {
    case 1: *effect = 0x36; break;
    case 2: *effect = 0x38; break;
    case 3: *effect = 0x37; break;
    case 4: *effect = 0x39; break;
    case 5: *effect = 0x3b; break;
    case 6: *effect = 0x3a; break;
    }
}

static void effect_by_level_if_unset(int level, int *effect)
{
    if (*effect == 0xc)
        effect_by_level(level, effect);
}

void session_get_cast_effect(struct Session *self, int skill, int *effect, int *action,
                                       int level, int job)
{
    int begin;

    *effect = 0x10;
    *action = 8;
    begin = session_get_begin_effect_id(skill);
    switch (skill) {
    case 0xbbe: case 0xbc1:
        *action = 0x24;
        break;
    case 0xbc3: case 0xbc5: case 0xbc6: case 0xbc7: case 0xbc8: case 0xbc9:
    case 0xbca: case 0xbcd: case 0xbd2: case 0xbd3: case 0xbd5:
        *action = 0x25;
        break;
    }
    if (begin != -1) {
        *effect = begin;
        return;
    }
    switch (skill) {
    case 0x3d: case 0x98: case 0x3ec:
        *effect = -1;
    case 0x95:
        *action = 0xb;
        effect_by_level_if_unset(level, effect);
        return;
    case 0x39:
        *effect = 0x90;
    case 0x5: case 0x7: case 0x2a: case 0x2e: case 0x6e: case 0x1f3:
    case 0x3f1:
        *action = 0x2;
        effect_by_level_if_unset(level, effect);
        return;
    case 0x3b:
        *effect = -1;
        *action = 0xc;
        effect_by_level_if_unset(level, effect);
        return;
    case 0x2d:
        *effect = 0x2b;
        return;
    case 0x2f:
        *effect = -1;
        *action = 0x12;
        effect_by_level_if_unset(level, effect);
        return;
    case 0xf3: case 0xf4: case 0xf7:
        *action = 0x0;
    case 0x145: case 0x147: case 0x148: case 0x149: case 0x14a: case 0x14e:
    case 0x150: case 0x198: case 0x199: case 0x19a: case 0x13c7: case 0x13c8:
    case 0x13c9:
        *effect = 0x156;
        return;
    case 0x97:
        *effect = -1;
        *action = 0x5;
        effect_by_level_if_unset(level, effect);
        return;
    case 0x10f:
        *effect = job < 0xfb0 ? 0x148 : 0x1fe;
        effect_by_level_if_unset(level, effect);
        return;
    case 0xe6: case 0xe7: case 0xe8: case 0xea: case 0xeb: case 0xec:
    case 0xed: case 0x10c: case 0x1df:
        *action = 0x0;
    case 0x9: case 0xa: case 0xb: case 0xc: case 0xd: case 0xe:
    case 0xf: case 0x10: case 0x11: case 0x12: case 0x13: case 0x14:
    case 0x15: case 0x16: case 0x17: case 0x18: case 0x19: case 0x1a:
    case 0x1b: case 0x1c: case 0x1e: case 0x1f: case 0x20: case 0x21:
    case 0x23: case 0x36: case 0x42: case 0x43: case 0x44: case 0x45:
    case 0x46: case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c:
    case 0x4d: case 0x4e: case 0x4f: case 0x50: case 0x51: case 0x52:
    case 0x53: case 0x54: case 0x55: case 0x56: case 0x57: case 0x58:
    case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x6c: case 0x9c:
    case 0x9d: case 0xfe: case 0xff: case 0x100: case 0x105: case 0x106:
    case 0x10a: case 0x10b: case 0x115: case 0x117: case 0x118: case 0x119:
    case 0x11a: case 0x11b: case 0x11d: case 0x11e: case 0x11f: case 0x120:
    case 0x121: case 0x122: case 0x133: case 0x135: case 0x136: case 0x138:
    case 0x139: case 0x153: case 0x154: case 0x155: case 0x156: case 0x16d:
    case 0x175: case 0x176: case 0x177: case 0x190: case 0x191: case 0x192:
    case 0x193: case 0x195: case 0x1e2: case 0x1e4: case 0x29d: case 0x2b6:
    case 0x2c3: case 0x2d3: case 0x2d9: case 0x2db: case 0x2e2: case 0x2e4:
    case 0x2e5: case 0x301: case 0x302: case 0x3ee: case 0x3f7: case 0x7d1:
    case 0x7d4: case 0x7d6: case 0x7d8: case 0x7db: case 0x7dc: case 0x7de:
    case 0x7df: case 0x7e2: case 0x7e3: case 0x7e4: case 0x7e5: case 0x7f4:
    case 0x7f8: case 0x7fb: case 0x7fc: case 0x7fe: case 0x803: case 0x805:
    case 0x806: case 0x809: case 0x8fd: case 0x9e5: case 0x9e6: case 0x9e8:
    case 0x9e9: case 0xa13: case 0xa18: case 0x138c: case 0x13a4: case 0x13b2:
    case 0x201e:
        *effect = 0xc;
        effect_by_level(level, effect);
        return;
    case 0x1de: case 0x1ea:
        *action = 0x1c;
        *effect = 0xc;
        effect_by_level(level, effect);
        return;
    case 0x185:
        *action = 0x0;
        *effect = 0x4ab;
        return;
    case 0x1db:
        *action = 0x0;
    case 0x8d: case 0x158: case 0x159: case 0x15a: case 0x15e: case 0x17b:
    case 0x196: case 0x295: case 0x2c8:
        *effect = 0x3b;
        return;
    case 0x14f:
        *effect = 0x157;
        return;
    case 0x17e: case 0x18a:
        *action = 0x12;
    case 0x132: case 0x134: case 0x137: case 0x169: case 0x16a: case 0x16f:
    case 0x17f: case 0x1e0: case 0x1e3: case 0x1e9: case 0x2b3: case 0x7dd:
    case 0x802: case 0x9d3: case 0xa16: case 0xa19: case 0xa1b: case 0x13d1:
        *effect = 0x3a;
        return;
    case 0x18d:
        *effect = 0x1f6;
        *action = 0xb;
        effect_by_level_if_unset(level, effect);
        return;
    case 0x1a9: case 0x1ed:
        *action = 0x0;
    case 0x13d: case 0x13f: case 0x140: case 0x141: case 0x142: case 0x19b:
    case 0x1ab: case 0x1b2: case 0x1bc: case 0x1bd: case 0x1bf: case 0x1c0:
    case 0x1c1: case 0x1c2: case 0x1c3: case 0x1c4: case 0x1c5: case 0x1c6:
    case 0x1c7: case 0x1c8: case 0x1c9: case 0x1ca: case 0x1cc: case 0x1cd:
    case 0x1ce: case 0x1cf: case 0x1d0: case 0x1d1: case 0x1d3: case 0x1d4:
    case 0x1d5: case 0x1d6: case 0x1d7: case 0x1d8: case 0x1ee: case 0x23c:
    case 0x23d: case 0x23e: case 0x23f: case 0x7f7: case 0x8fb: case 0x8fc:
    case 0x8ff: case 0xa24: case 0xa25: case 0xa26: case 0xa27: case 0xa2d:
    case 0xa2f: case 0xa30:
        *effect = 0x1b9;
        return;
    case 0x1be:
        *action = 0xc;
        *effect = 0xc;
        effect_by_level(level, effect);
        return;
    case 0x1f0:
        *action = 0x7;
        *effect = 0x23d;
        return;
    case 0x1f1:
        *action = 0x7;
        *effect = 0x23e;
        return;
    case 0x1f2:
        *action = 0x7;
        *effect = 0x23f;
        return;
    case 0x1f7: case 0x1f8: case 0x1f9: case 0x200: case 0x201: case 0x202:
    case 0x203: case 0x205:
        *action = 0x29;
        *effect = 0x1b9;
        return;
    case 0x22: case 0x2b1:
        *effect = 0xc;
        *action = 0x0;
        effect_by_level_if_unset(level, effect);
        return;
    case 0x20d: case 0x214: case 0x21f:
        *action = 0x25;
        *effect = 0x1b9;
        return;
    case 0x216: case 0x217: case 0x218:
        *action = 0x25;
    case 0x3fa:
        *effect = 0x37;
        return;
    case 0x219: case 0x21a: case 0x21b:
        *action = 0x25;
        *effect = 0x36;
        return;
    case 0x21c: case 0x21d: case 0x21e:
        *action = 0x25;
    case 0x3fb:
        *effect = 0x39;
        return;
    case 0x206: case 0x207: case 0x208: case 0x209:
        *action = 0x2a;
        *effect = 0x1b9;
        return;
    case 0x3e9: case 0x3f2: case 0x3f6:
        *action = 0x0;
        *effect = 0x3a;
        return;
    case 0x804:
        *effect = 0x618;
        return;
    case 0x38: case 0x3a: case 0x3e: case 0x807:
        *effect = -1;
        *action = 0x2;
        effect_by_level_if_unset(level, effect);
        return;
    case 0x7d2:
        *effect = 0xc;
        *action = 0x2;
        effect_by_level_if_unset(level, effect);
        return;
    case 0x73: case 0x74: case 0x75: case 0x76: case 0x77: case 0x78:
    case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x2b0:
    case 0x8bf: case 0x8cd: case 0x8ce:
        *action = 0x5;
        effect_by_level_if_unset(level, effect);
        return;
    case 0x22b: case 0x8f2: case 0x900:
        *action = 0x0;
        *effect = 0x1f5;
        return;
    case 0x8f4: case 0x8f5: case 0x8f6: case 0x8f7: case 0x8f8: case 0x8f9:
    case 0x8fa:
        *action = 0x7;
        *effect = 0x1b9;
        return;
    case 0x8fe:
        *effect = 0x614;
        return;
    case 0x2e8: case 0x98c:
        *action = -1;
        *effect = 0x39;
        return;
    case 0x98e:
        *action = -1;
        *effect = 0x680;
        return;
    case 0x996:
        *action = 0x7;
        *effect = 0x61d;
        return;
    case 0x89c:
        *effect = 0x5a9;
        *action = 0x7;
        effect_by_level_if_unset(level, effect);
        return;
    case 0x8a5:
        *effect = 0x58a;
        *action = 0x7;
        effect_by_level_if_unset(level, effect);
        return;
    case 0x8a9:
        *effect = 0x325;
        *action = 0x7;
        effect_by_level_if_unset(level, effect);
        return;
    case 0x8ae: case 0x8af: case 0x8b0: case 0x8b5:
        *effect = g_session1.map_rule_flags[0] == 1 ? 0x596 : 0x4ec;
        begin_wave("effect\\EF_BeginSpell.wav");
        *action = 0x7;
        effect_by_level_if_unset(level, effect);
        return;
    case 0x8f0:
        *effect = 0x1b9;
        *action = 0x12;
        effect_by_level_if_unset(level, effect);
        return;
    case 0x8ef:
        *effect = 0x33a;
        *action = 0x7;
        effect_by_level_if_unset(level, effect);
        return;
    case 0x1d: case 0x2b2: case 0x2c4: case 0x2d0: case 0x2d8: case 0x899:
    case 0x89b: case 0x89d: case 0x89f: case 0x8a2: case 0x8a3: case 0x8a8:
    case 0x8b6: case 0x8b9: case 0x8bd: case 0x8c3: case 0x8c4: case 0x8c6:
        *effect = 0xc;
        *action = 0x7;
        effect_by_level_if_unset(level, effect);
        return;
    case 0x8a1: case 0x8c0: case 0x8ee:
        *effect = 0x1b9;
        *action = 0x7;
        effect_by_level_if_unset(level, effect);
        return;
    case 0x88: case 0x2e1: case 0x7e6: case 0x808: case 0x89a: case 0x8a4:
    case 0x8a6: case 0x8bc: case 0x90d: case 0x911:
        *effect = -1;
        *action = 0x7;
        effect_by_level_if_unset(level, effect);
        return;
    case 0x913:
        *effect = 0x6ad;
        *action = 0x7;
        effect_by_level_if_unset(level, effect);
        return;
    case 0x912:
        *effect = 0x33e;
        *action = 0x7;
        effect_by_level_if_unset(level, effect);
        return;
    case 0x90c:
        *effect = 0x372;
        *action = 0x7;
        effect_by_level_if_unset(level, effect);
        return;
    case 0x8e8:
        *action = 0x2;
        *effect = 0x307;
        return;
    case 0x8d3:
        *action = 0x2;
        *effect = 0xc;
        effect_by_level(level, effect);
        return;
    case 0x8d5:
        *action = 0x2;
    case 0x33: case 0x37: case 0x3c: case 0x3f: case 0x40: case 0x5d:
    case 0x9e: case 0x9f: case 0xa0: case 0xa1: case 0xa2: case 0xa3:
    case 0xa4: case 0xa5: case 0xa6: case 0xa7: case 0xa8: case 0xa9:
    case 0xaa: case 0xab: case 0xac: case 0xad: case 0xae: case 0xaf:
    case 0xb0: case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5:
    case 0xb6: case 0xb7: case 0xb8: case 0xb9: case 0xba: case 0xbb:
    case 0xbc: case 0xbd: case 0xbe: case 0xbf: case 0xc0: case 0x102:
    case 0x110: case 0x111: case 0x152: case 0x15b: case 0x170: case 0x172:
    case 0x173: case 0x174: case 0x18e: case 0x18f: case 0x1ef: case 0x28e:
    case 0x297: case 0x29a: case 0x29b: case 0x29c: case 0x2a3: case 0x2a4:
    case 0x2a6: case 0x2d1: case 0x2d4: case 0x2f5: case 0x2f6: case 0x2f7:
    case 0x2f8: case 0x2fa: case 0x30e: case 0x30f: case 0x3f8: case 0x7d5:
    case 0x7f6: case 0x8e6:
        *effect = -1;
        return;
    case 0x2e0: case 0x2e3: case 0x2e6: case 0x8d0: case 0x8d2: case 0x8d4:
    case 0x8d6: case 0x8dc: case 0x98f: case 0x990: case 0x991: case 0x992:
    case 0x993: case 0x994: case 0x995: case 0x997: case 0x998: case 0x999:
    case 0x99a: case 0x99b: case 0x99c:
        *action = -1;
        *effect = 0xc;
        effect_by_level(level, effect);
        return;
    case 0x8db:
        *action = -1;
        *effect = 0x307;
        return;
    case 0x2e9: case 0x8de: case 0x98b:
        *action = -1;
        *effect = 0x37;
        return;
    case 0x8e0:
        *action = -1;
        *effect = 0x36;
        return;
    case 0x8df: case 0x8e1: case 0x8e3:
        *action = -1;
        *effect = 0x3a;
        return;
    case 0x8e2:
        *action = -1;
        *effect = 0x1b9;
        return;
    case 0x92e: case 0x92f: case 0x930: case 0x971: case 0x973: case 0x974:
    case 0x976: case 0x978: case 0x979:
        *action = 0x10;
        *effect = 0x348;
        if (skill == 0x973)
            begin_wave("effect\\나락의노래.wav");
        else if (skill == 0x930)
            begin_wave("effect\\달빛세레나데.wav");
        else
            return;
        effect_by_level_if_unset(level, effect);
        return;
    case 0x96e:
        *action = 0x10;
        *effect = 0x678;
        return;
    case 0x3f3: case 0x96d:
        *action = 0x10;
        *effect = -1;
        return;
    case 0x972:
        *action = 0x10;
        *effect = 0x506;
        return;
    case 0x975: case 0x977:
        *action = 0x10;
        *effect = 0x349;
        return;
    case 0x94d: case 0x94e:
        *action = 0x10;
        *effect = 0x34c;
        return;
    case 0x97a: case 0x97b: case 0x97c: case 0x97e: case 0x97f: case 0x980:
    case 0x981: case 0x982:
        *action = 0x10;
        *effect = 0x34b;
        if (skill != 0x97f)
            return;
        begin_wave("effect\\레라드의이슬.wav");
        effect_by_level_if_unset(level, effect);
        return;
    case 0x97d:
        *effect = -1;
        *action = 0x10;
        effect_by_level_if_unset(level, effect);
        return;
    case 0x94f:
        *action = 0x10;
        *effect = 0x1b9;
        return;
    case 0x916: case 0x917: case 0x918: case 0x919: case 0x91a: case 0x91c:
    case 0x922: case 0x924: case 0x925: case 0x928: case 0x929: case 0x92a:
    case 0x92b: case 0x92c: case 0x9d6:
        *action = -1;
        *effect = 0x33f;
        return;
    case 0xa04:
        *action = 0x2e;
        *effect = -1;
        return;
    case 0x2df: case 0x9ac: case 0x9b0: case 0x9be: case 0x9bf: case 0x9c0:
    case 0x9c1:
        *action = -1;
        *effect = 0x38f;
        return;
    case 0x9ae:
        *action = -1;
        *effect = 0x68d;
        return;
    case 0x927: case 0x9bc: case 0x9d5:
        *action = -1;
        *effect = -1;
        return;
    case 0x9b1: case 0x9b5:
        *action = 0xc;
        *effect = 0x38f;
        return;
    case 0x9af: case 0x9b3: case 0x9b6: case 0x9ba:
        *action = 0x7;
        *effect = 0x38f;
        return;
    case 0x9b2:
        *action = 0x7;
        *effect = -1;
        return;
    case 0x8c1: case 0x8c2: case 0x9ad: case 0x9d8:
        *effect = -1;
        *action = -1;
        effect_by_level_if_unset(level, effect);
        return;
    case 0xa15:
        *action = 0x13;
        *effect = 0x3a;
        return;
    case 0xa28: case 0xa29: case 0xa2a: case 0xa2b: case 0xa2c: case 0xa2e:
    case 0xa32:
        *effect = 0x1c6;
        return;
    case 0x1aa: case 0x139f:
        *action = 0x13;
        *effect = -1;
        return;
    case 0x13a2: case 0x13a3: case 0x13a8: case 0x13a9: case 0x13ab: case 0x13ac:
    case 0x13ae: case 0x13b0: case 0x13b3:
        *action = 0x32;
        *effect = -1;
        return;
    case 0x7f9: case 0x7fa: case 0x7fd: case 0x7ff: case 0x800: case 0x13d0:
        *effect = 0x33e;
        return;
    case 0x89e: case 0x8b7: case 0x909: case 0x90a: case 0x90b: case 0x90e:
    case 0x90f: case 0x910: case 0x914: case 0x915: case 0x13d6:
        *effect = 0x3a;
        *action = 0x7;
        effect_by_level_if_unset(level, effect);
        return;
    case 0x7f0: case 0x8ba: case 0x138a:
        begin_wave("effect\\EF_BeginSpell.wav");
        effect_by_level_if_unset(level, effect);
        return;
    case 0xc4: case 0xd1: case 0xf9: case 0x10e: case 0x15d: case 0x201c:
        *action = 0x0;
        *effect = -1;
        return;
    case 0x20d1: case 0x20e9: case 0x20ea: case 0x20ec:
        *action = 0x7;
        *effect = 0x37;
        return;
    case 0x20ee: case 0x20ef: case 0x20f1:
        *action = 0x7;
        *effect = 0x36;
        return;
    case 0x20f2: case 0x20f3:
        *action = 0x7;
        *effect = 0x39;
        return;
    case 0x20fa:
        *action = 0x7;
    case 0x3f9:
        *effect = 0x38;
        return;
    default:
    case 0x1: case 0x2: case 0x3: case 0x4: case 0x6: case 0x8:
    case 0x24: case 0x25: case 0x26: case 0x27: case 0x28: case 0x29:
    case 0x2b: case 0x2c: case 0x30: case 0x31: case 0x32: case 0x34:
    case 0x35: case 0x5e: case 0x5f: case 0x60: case 0x61: case 0x62:
    case 0x63: case 0x64: case 0x65: case 0x66: case 0x67: case 0x68:
    case 0x69: case 0x6a: case 0x6b: case 0x6d: case 0x6f: case 0x70:
    case 0x71: case 0x72: case 0x7e: case 0x7f: case 0x80: case 0x81:
    case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87:
    case 0x89: case 0x8a: case 0x8b: case 0x8c: case 0x1cb: case 0x1e6:
    case 0x7f1:
        effect_by_level_if_unset(level, effect);
        return;
    case 0x3f0:
        *effect = 0x36;
        return;
    }
}
