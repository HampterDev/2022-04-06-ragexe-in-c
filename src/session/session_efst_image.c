#include "contents/client_lua_calls.h"
#include "session/csession.h"
#include "session/session_jobs.h"
#include "session/session_skills.h"

#include <string.h>

static int drawn_job_takes_i_p_icon(void)
{
    if (session_get_drawn_job(&g_session1) >= 0xfce && session_get_drawn_job(&g_session1) <= 0xfd1)
        return 1;
    if (session_get_drawn_job(&g_session1) == 0x1081 || session_get_drawn_job(&g_session1) == 0x1082
        || session_get_drawn_job(&g_session1) == 0x108e || session_get_drawn_job(&g_session1) == 0x1083)
        return 1;
    if (session_get_drawn_job(&g_session1) >= 0x108f && session_get_drawn_job(&g_session1) <= 0x1094)
        return 1;
    return session_get_drawn_job(&g_session1) == 0x10ce || session_get_drawn_job(&g_session1) == 0x10cf
        || session_get_drawn_job(&g_session1) == 0x10dc;
}

const char *session_get_efst_image(struct Session *self, int efst, int pass)
{
    const char *name = "";

    client_lua_call(self->luaState, "GetEFSTImgFileName", "dd>s", efst, pass, &name);
    if (strcmp(name, "") != 0)
        return name;
    switch (pass) {
    case 0:
        switch (efst) {
        case 0x2fa:
            return "effect\\SWORDCLAN.TGA";
        case 0x2fb:
            return "effect\\ARCWANDCLAN.TGA";
        case 0x2fc:
            return "effect\\GOLDENMACECLAN.TGA";
        case 0x2fd:
            return "effect\\CROSSBOWCLAN.TGA";
        }
        break;
    case 1:
        switch (efst) {
        case 0x20:
            return "effect\\갑옷파괴.tga";
        case 0x22:
            return "effect\\환각.tga";
        case 0x8:
            return "effect\\콰그마이어.tga";
        case 0x21:
            return "effect\\무기파괴.tga";
        case 0x23:
            return "effect\\무게" "50이상.tga";
        case 0x24:
            return "effect\\무게" "90이상.tga";
        case 0x32:
            return "effect\\스트립웨폰.tga";
        case 0x33:
            return "effect\\스트립쉴드.tga";
        case 0x34:
            return "effect\\스트립아머.tga";
        case 0x35:
            return "effect\\스트립헬름.tga";
        case 0x5a:
            if (drawn_job_takes_i_p_icon())
                return "effect\\i_p_FIRE.tga";
            return "effect\\프레임런쳐.tga";
        case 0xfa:
            return "effect\\exp.tga";
        case 0xfb:
            return "effect\\death.tga";
        case 0xfc:
            return "effect\\item.tga";
        case 0xd: case 0x191:
            return "effect\\민첩성감소.tga";
        case 0xf7: case 0xf8: case 0xf9: case 0x139:
            return "effect\\gogi.tga";
        case 0x12a:
            return "effect\\멘탈포션.tga";
        case 0xf5: case 0x113: case 0x1ec:
            return "effect\\Int_gogi.tga";
        case 0xf1: case 0x10f: case 0x1eb:
            return "effect\\Str_gogi.tga";
        case 0xf2: case 0x110: case 0x1ef:
            return "effect\\Agi_gogi.tga";
        case 0xf3: case 0x111: case 0x1ed:
            return "effect\\Vit_gogi.tga";
        case 0xf4: case 0x112: case 0x1ee:
            return "effect\\Dex_gogi.tga";
        case 0xf6: case 0x114: case 0x1f0:
            return "effect\\Luk_gogi.tga";
        case 0x1dc:
            return "effect\\npc_hellpower.tga";
        case 0x26d:
            return "effect\\변신.tga";
        case 0x138: case 0x27c:
            return "effect\\job.tga";
        case 0x27d:
            return "effect\\NORECOVER.TGA";
        case 0x296: case 0x297:
            return "effect\\icon02.tga";
        case 0x298:
            return "effect\\rwcscroll.tga";
        case 0x2da:
            return "effect\\darkcrow.tga";
        case 0x2d1:
            return "effect\\REBOUND.tga";
        }
        break;
    case 2:
        switch (efst) {
        case 0x130:
            return "effect\\npc_hellpower.tga";
        case 0x1:
            return "effect\\인듀어.tga";
        case 0x6:
            return "effect\\인찬트포이즌.tga";
        case 0x5:
            return "effect\\클로킹.tga";
        case 0x2:
            return "effect\\투핸드퀴큰.tga";
        case 0x1a:
            return "effect\\맥시마이즈파워.tga";
        case 0x17: case 0x93:
            return "effect\\아드레날린러쉬.tga";
        case 0x7:
            return "effect\\포이즌리액트.tga";
        case 0x3a:
            return "effect\\오토가드.tga";
        case 0x41:
            return "effect\\오토스펠.tga";
        case 0x0:
            return "effect\\프로보크.tga";
        case 0x19:
            return "effect\\오버트러스트.tga";
        case 0x18:
            return "effect\\웨폰퍼펙션.tga";
        case 0x4:
            return "effect\\하이딩.tga";
        case 0x44:
            return "effect\\스피어퀴큰.tga";
        case 0x3e:
            return "effect\\디펜더.tga";
        case 0x3b:
            return "effect\\리플렉트쉴드.tga";
        case 0x5b:
            if (drawn_job_takes_i_p_icon())
                return "effect\\i_p_WATER.tga";
            return "effect\\프로스트웨폰.tga";
        case 0x5c:
            if (drawn_job_takes_i_p_icon())
                return "effect\\i_p_WIND.tga";
            return "effect\\라이트닝로더.tga";
        case 0x7c:
            return "effect\\출혈상태.tga";
        case 0x7d:
            return "effect\\관절공격.tga";
        case 0x68:
            return "effect\\ICON10.tga";
        case 0x73:
            return "effect\\ICON09.tga";
        case 0x74:
            return "effect\\ICON06.tga";
        case 0x76:
            return "effect\\ICON07.tga";
        case 0x75:
            return "effect\\ICON03.tga";
        case 0x78:
            return "effect\\ICON04.tga";
        case 0x79: case 0x7a:
            return "effect\\ICON01.tga";
        case 0x84:
            return "effect\\금강불괴.tga";
        case 0x87:
            return "effect\\i_STORMKICK.tga";
        case 0x89:
            return "effect\\i_DOWNKICK.tga";
        case 0x8b:
            return "effect\\i_TURNKICK.tga";
        case 0x8d:
            return "effect\\i_COUNTER.tga";
        case 0x8f:
            return "effect\\i_DODGE.tga";
        case 0x91:
            return "effect\\i_RUN.tga";
        case 0x92:
            return "effect\\i_p_DARK.tga";
        case 0x94:
            return "effect\\i_p_TELE.tga";
        case 0x72:
            return "effect\\I_치독.tga";
        case 0xb5:
            return "effect\\I_프리저브.tga";
        case 0xb6:
            return "effect\\I_체이스힘.tga";
        case 0x70:
            return "effect\\I_장판.tga";
        case 0xba:
            return "effect\\I_더블캐스팅.tga";
        case 0xbc:
            return "effect\\I_오버맥스.tga";
        case 0xbf: case 0x23d:
            return "effect\\I_타로카드.tga";
        case 0xa9:
            return "effect\\i_SUNCOMFORT.tga";
        case 0xaa:
            return "effect\\i_MOONCOMFORT.tga";
        case 0xab:
            return "effect\\i_STARCOMFORT.tga";
        case 0x9c:
            return "effect\\i_KAIZEL.tga";
        case 0x9d:
            return "effect\\i_KAAHI.tga";
        case 0x9e:
            return "effect\\i_KAUPE.tga";
        case 0xa1: case 0x19e:
            return "effect\\i_ONEHAND.tga";
        case 0xc5:
            return "effect\\i_shrink.tga";
        case 0xc6:
            return "effect\\i_sightblaster.tga";
        case 0xc8: case 0xc9:
            return "effect\\i_closeconfine.tga";
        case 0xcf:
            return "effect\\i_BUNSIN.tga";
        case 0xce:
            return "effect\\i_MAEMI.tga";
        case 0xd0:
            return "effect\\i_NEN.tga";
        case 0xd1:
            return "effect\\i_ADJUSTMENT.tga";
        case 0xd2:
            return "effect\\i_ACCURACY.tga";
        case 0xcb:
            return "effect\\i_MADNESS.tga";
        case 0xcc:
            return "effect\\i_FEVER.tga";
        case 0x96:
            return "effect\\PLUSATTACKPOWER.tga";
        case 0x107:
            return "effect\\죽은척하기.tga";
        case 0x61: case 0xcd: case 0xd4: case 0xd5: case 0xd6: case 0xd7:
        case 0xd8: case 0xd9: case 0xda: case 0xdb: case 0xdc: case 0xdd:
        case 0xde: case 0xdf: case 0xe0: case 0xe1: case 0xe2: case 0xe3:
        case 0xe4: case 0xe5: case 0xe6: case 0xe7: case 0xe8: case 0xe9:
        case 0xea: case 0xeb: case 0xec: case 0xef: case 0xf0: case 0xfe:
        case 0xff: case 0x100: case 0x101: case 0x102: case 0x103: case 0x104:
        case 0x105: case 0x106: case 0x108: case 0x109: case 0x10a: case 0x10b:
        case 0x10c: case 0x10d: case 0x10e:
            return "effect\\글로리아.tga";
        case 0x11a:
            return "effect\\slowcast.tga";
        case 0x11e:
            return "effect\\criticalwound.tga";
        case 0x122:
            return "effect\\물리방어포션.tga";
        case 0x82: case 0x13a:
            return "effect\\엄마아빠사랑해요.tga";
        case 0x1a7: case 0x243:
            return "effect\\ICON02.tga";
        case 0x12c:
            return "effect\\SP소모량감소포션.tga";
        case 0x12d:
            return "effect\\올상태저항포션.tga";
        case 0x179:
            return "effect\\뉴트럴배리어.tga";
        case 0x176:
            return "effect\\셰이프쉬프트.tga";
        case 0x17b:
            return "effect\\스텔스필드.tga";
        case 0x1c2:
            return "effect\\애널라이즈.tga";
        case 0x169:
            return "effect\\엑셀레이션.tga";
        case 0x150:
            return "effect\\레노바티오.tga";
        case 0x1d8:
            return "effect\\세크라멘트.tga";
        case 0x194:
            return "effect\\데들리인펙트.tga";
        case 0x181:
            return "effect\\리프로듀스.tga";
        case 0x198:
            return "effect\\Masquerade_ENERVATION.tga";
        case 0x199:
            return "effect\\Masquerade_GROOMY.tga";
        case 0x19b:
            return "effect\\Masquerade_IGNORANCE.tga";
        case 0x19c:
            return "effect\\Masquerade_LAZINESS.tga";
        case 0x19f:
            return "effect\\Masquerade_UNLUCKY.tga";
        case 0x1a2:
            return "effect\\Masquerade_WEAKNESS.tga";
        case 0x18f:
            return "effect\\바디페인팅.tga";
        case 0x18a:
            return "effect\\쉐도우폼.tga";
        case 0x1a5:
            return "effect\\스트립액세서리.tga";
        case 0x189:
            return "effect\\오토쉐도우스펠.tga";
        case 0x193:
            return "effect\\인비지빌리티.tga";
        case 0x175:
            return "effect\\오버히트.tga";
        case 0x177:
            return "effect\\인프라레드스캔.tga";
        case 0x1b5:
            return "effect\\다이아몬드더스트.tga";
        case 0x16a:
            return "effect\\호버링.tga";
        case 0x127:
            return "effect\\연마제.tga";
        case 0x124:
            return "effect\\회복력향상포션.tga";
        case 0x126: case 0x242:
            return "effect\\중형생명수.tga";
        case 0x128:
            return "effect\\환영의술잔.tga";
        case 0x123:
            return "effect\\마법방어포션.tga";
        case 0x12e:
            return "effect\\캐시갑옷속성부여.tga";
        case 0x17d: case 0x17f: case 0x2b9:
            return "effect\\EFST_ATK.tga";
        case 0x13c:
            return "effect\\RK_ENCHANT.tga";
        case 0x17e: case 0x180: case 0x2bb:
            return "effect\\EFST_DEF.tga";
        case 0x13e:
            return "effect\\RK_NAUTHIZ.tga";
        case 0x141:
            return "effect\\RK_ISHA.tga";
        case 0x142:
            return "effect\\RK_EISIR.tga";
        case 0x143:
            return "effect\\RK_URUZ.tga";
        case 0x13f:
            return "effect\\RK_TURISUSS.tga";
        case 0x140:
            return "effect\\RK_HAGALAZ.tga";
        case 0x184:
            return "effect\\str_gogi.tga";
        case 0x185:
            return "effect\\int_gogi.tga";
        case 0x162:
            return "effect\\MARSHOFABYSS.tga";
        case 0x15f:
            return "effect\\FROSTMISTY.tga";
        case 0x163:
            return "effect\\RECOGNIZEDSPELL.tga";
        case 0x160:
            return "effect\\RA_FEARBREEZE.tga";
        case 0x155: case 0x157:
            return "effect\\감각마비독.tga";
        case 0x15d:
            return "effect\\거머리.tga";
        case 0x15c:
            return "effect\\망각의재.tga";
        case 0x15a:
            return "effect\\상처오염독.tga";
        case 0x158:
            return "effect\\쇠약독.tga";
        case 0x156:
            return "effect\\신경마비독.tga";
        case 0x15b:
            return "effect\\열병독.tga";
        case 0x159:
            return "effect\\웃음버섯.tga";
        case 0x151:
            return "effect\\웨폰블로킹.tga";
        case 0x14d:
            return "effect\\포이즈닝웨폰.tga";
        case 0x1bf:
            return "effect\\달빛의세레나데.tga";
        case 0x1c3:
            return "effect\\레라드의이슬.tga";
        case 0x1b7:
            return "effect\\마나의노래.tga";
        case 0x1bb:
            return "effect\\메아리의노래.tga";
        case 0x1c4:
            return "effect\\멜로디오브싱크.tga";
        case 0x1c5:
            return "effect\\비욘드오브워크라이.tga";
        case 0x1c0:
            return "effect\\새터데이나이트피버.tga";
        case 0x1b6:
            return "effect\\수줍은하루의우울.tga";
        case 0x1b4:
            return "effect\\순환하는자연의소리.tga";
        case 0x1ad:
            return "effect\\스윙댄스.tga";
        case 0x1b3:
            return "effect\\안식의자장가.tga";
        case 0x1c6:
            return "effect\\언리미티드허밍보이스.tga";
        case 0x1ae:
            return "effect\\연인들을위한심포니.tga";
        case 0x1b9:
            return "effect\\워그와함께춤을.tga";
        case 0x1ba:
            return "effect\\풍차를향해돌격.tga";
        case 0x1bc:
            return "effect\\하모나이즈.tga";
        case 0x1b8:
            return "effect\\클라우드킬.tga";
        case 0x1be:
            return "effect\\워머.tga";
        case 0x1bd:
            return "effect\\스트라이킹.tga";
        case 0x19d:
            return "effect\\섬전보.tga";
        case 0x19a:
            return "effect\\잠룡승천.tga";
        case 0x1a9:
            return "effect\\점혈구.tga";
        case 0x1aa:
            return "effect\\점혈반.tga";
        case 0x1ab:
            return "effect\\점혈활.tga";
        case 0x186:
            return "effect\\LG_REFLECTDAMAGE.tga";
        case 0x18c:
            return "effect\\LG_SHIELDSPELL_방.tga";
        case 0x18d:
            return "effect\\LG_SHIELDSPELL_마.tga";
        case 0x18e:
            return "effect\\LG_SHIELDSPELL_연.tga";
        case 0x192:
            return "effect\\LG_PRESTIGE.tga";
        case 0x195:
            return "effect\\LG_BANDING.tga";
        case 0x197:
            return "effect\\LG_INSPIRATION.tga";
        case 0x1cf:
            return "effect\\가시나무덫.tga";
        case 0x1cd:
            return "effect\\카트부스트.tga";
        case 0x187:
            return "effect\\포스오브뱅가드.tga";
        case 0x129:
            return "effect\\대환단.tga";
        case 0x12b:
            return "effect\\태청단.tga";
        case 0x172:
            return "effect\\MVPCARD_ORCHERO.tga";
        case 0x173:
            return "effect\\MVPCARD_ORCLORD.tga";
        case 0x170:
            return "effect\\MVPCARD_TAOGUNKA.tga";
        case 0x171:
            return "effect\\MVPCARD_MISTRESS.tga";
        case 0x247:
            return "effect\\all_odins_power.tga";
        case 0x6a: case 0x278: case 0x279: case 0x27a: case 0x27b:
            return "effect\\ICON08.tga";
        case 0x182: case 0x183: case 0x263: case 0x2ba:
            return "effect\\EFST_MATK.tga";
        case 0x125: case 0x29e:
            return "effect\\소형생명수.tga";
        case 0x26e:
            return "effect\\앉기.tga";
        case 0x27e: case 0x280:
            return "effect\\SETDEF.TGA";
        case 0x2b7:
            return "effect\\ArrowN.tga";
        case 0x2bc:
            return "effect\\EXP.tga";
        case 0x14e: case 0x365:
            return "effect\\할루시네이션워크.tga";
        case 0x6e: case 0x1d9: case 0x461:
            return "effect\\ICON05.tga";
        }
        break;
    case 3:
        switch (efst) {
        case 0x1d:
            return "effect\\죽은척하기.tga";
        case 0x3:
            return "effect\\집중력향상.tga";
        case 0x1c:
            return "effect\\팔콘.tga";
        case 0x1e:
            return "effect\\고성지르기.tga";
        case 0x1f:
            return "effect\\에너지코트.tga";
        case 0x25: case 0x26: case 0x27: case 0x28:
            return "effect\\공속물약.tga";
        case 0x36:
            return "effect\\케미컬프로텍션[웨폰].tga";
        case 0x37:
            return "effect\\케미컬프로텍션[쉴드].tga";
        case 0x38:
            return "effect\\케미컬프로텍션[아머].tga";
        case 0x39:
            return "effect\\케미컬프로텍션[헬름].tga";
        case 0x5d:
            if (drawn_job_takes_i_p_icon())
                return "effect\\i_p_EARTH.tga";
            return "effect\\사이즈믹웨폰.tga";
        case 0x97:
            return "effect\\PLUSMAGICPOWER.tga";
        case 0x165:
            return "effect\\wolfmount.tga";
        case 0x1b: case 0x265:
            return "effect\\라이딩.tga";
        case 0x27f: case 0x281:
            return "effect\\SETMDEF.TGA";
        case 0x287:
            return "effect\\ASPDCASH.TGA";
        case 0x28a:
            return "effect\\ELDICASTES.TGA";
        }
        break;
    case 4:
        switch (efst) {
        case 0x15:
            return "effect\\글로리아.tga";
        case 0x13:
            return "effect\\기리에엘레이손.tga";
        case 0x16:
            return "effect\\렉스에테르나.tga";
        case 0xb:
            return "effect\\시그넘크루시스.tga";
        case 0x14:
            return "effect\\마니피캇.tga";
        case 0xa:
            return "effect\\블레싱.tga";
        case 0x12:
            return "effect\\성체강복.tga";
        case 0x10:
            return "effect\\수프라기움.tga";
        case 0xe:
            return "effect\\슬로우포이즌.tga";
        case 0x11: case 0x40:
            if (drawn_job_takes_i_p_icon())
                return "effect\\i_p_SAINT.tga";
            return "effect\\아스페르시오.tga";
        case 0x9:
            return "effect\\안젤루스.tga";
        case 0xf:
            return "effect\\임포시티오마누스.tga";
        case 0x3d:
            return "effect\\신의뜻.tga";
        case 0x57:
            return "effect\\STEELBODY.tga";
        case 0xc: case 0x29: case 0x2a: case 0x121:
            return "effect\\민첩성증가.tga";
        case 0x241:
            return "effect\\세라" "4.tga";
        case 0x244:
            return "effect\\에이라" "1.tga";
        case 0x245:
            return "effect\\에이라" "2.tga";
        case 0x248:
            return "effect\\에레노아" "1_1.tga";
        case 0x25d:
            return "effect\\디이터" "1.tga";
        case 0x25e:
            return "effect\\디이터" "2.tga";
        case 0x25f:
            return "effect\\디이터" "4.tga";
        case 0x260:
            return "effect\\디이터" "5.tga";
        case 0x26a:
            return "effect\\exp_말랑도_고양이캔.tga";
        case 0x292:
            return "effect\\PHIDEMON.tga";
        case 0x2d2:
            return "effect\\UNLIMIT.tga";
        case 0x2d3:
            return "effect\\KINGS_GRACE.tga";
        case 0x2cb:
            return "effect\\FRIGG_SONG.tga";
        case 0x2cc:
            return "effect\\OFFERTORIUM.tga";
        case 0x2cd:
            return "effect\\TELEKINESIS_INTENSE.tga";
        case 0x2d0:
            return "effect\\FULL_THROTTLE.tga";
        }
        break;
    }
    return NULL;
}
