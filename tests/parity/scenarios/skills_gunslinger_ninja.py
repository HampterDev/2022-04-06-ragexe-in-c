"""The skill sweep for the Gunslinger, the Ninja and the Super Novice (parity69): every gun with its
ammunition, the coins, the Ninja's throws and elemental arts, Final Strike, and the Super Novice,
whose skills the first classes cast.

Cast here, by class:
Gunslinger (24): GS_GLITTERING, GS_FLING, GS_TRIPLEACTION, GS_BULLSEYE, GS_MADNESSCANCEL, GS_ADJUSTMENT,
    GS_INCREASING, GS_MAGICALBULLET, GS_CRACKER, GS_TRACKING, GS_DISARM, GS_PIERCINGSHOT, GS_RAPIDSHOWER,
    GS_DESPERADO, GS_GATLINGFEVER, GS_DUST, GS_FULLBUSTER, GS_SPREADATTACK, GS_GROUNDDRIFT
Ninja (25): NJ_SYURIKEN, NJ_KUNAI, NJ_HUUMA, NJ_ZENYNAGE, NJ_TATAMIGAESHI, NJ_KASUMIKIRI, NJ_SHADOWJUMP,
    NJ_KIRIKAGE, NJ_UTSUSEMI, NJ_BUNSINJYUTSU, NJ_KOUENKA, NJ_BAKUENRYU, NJ_HYOUSENSOU, NJ_HYOUSYOURAKU,
    NJ_HUUJIN, NJ_RAIGEKISAI, NJ_KAMAITACHI, NJ_NEN, NJ_KAENSIN, NJ_SUITON, NJ_ISSEN
Super_Novice (23): none of its own
Not cast here:
SM_BASH (Super_Novice): cast as Swordsman
SM_PROVOKE (Super_Novice): cast as Swordsman
SM_MAGNUM (Super_Novice): cast as Swordsman
SM_ENDURE (Super_Novice): cast as Swordsman
MG_SIGHT (Super_Novice): cast as Magician
MG_NAPALMBEAT (Super_Novice): cast as Magician
MG_SAFETYWALL (Super_Novice): cast as Magician
MG_SOULSTRIKE (Super_Novice): cast as Magician
MG_COLDBOLT (Super_Novice): cast as Magician
MG_FROSTDIVER (Super_Novice): cast as Magician
MG_STONECURSE (Super_Novice): cast as Magician
MG_FIREBALL (Super_Novice): cast as Magician
MG_FIREWALL (Super_Novice): cast as Magician
MG_FIREBOLT (Super_Novice): cast as Magician
MG_LIGHTNINGBOLT (Super_Novice): cast as Magician
MG_THUNDERSTORM (Super_Novice): cast as Magician
AL_RUWACH (Super_Novice): cast as Acolyte
AL_PNEUMA (Super_Novice): cast as Acolyte
AL_TELEPORT (Super_Novice): cast as Acolyte
AL_WARP (Super_Novice): cast as Acolyte
AL_HEAL (Super_Novice): cast as Crusader
AL_INCAGI (Super_Novice): cast as Acolyte
AL_DECAGI (Super_Novice): cast as Acolyte
AL_HOLYWATER (Super_Novice): cast as Acolyte
AL_CRUCIS (Super_Novice): cast as Acolyte
AL_ANGELUS (Super_Novice): cast as Acolyte
AL_BLESSING (Super_Novice): cast as Acolyte
AL_CURE (Super_Novice): cast as Crusader
MC_IDENTIFY (Super_Novice): cast as Merchant
MC_VENDING (Super_Novice): cast as Merchant
MC_MAMMONITE (Super_Novice): cast as Merchant
AC_CONCENTRATION (Super_Novice): cast as Archer
TF_STEAL (Super_Novice): cast as Thief
TF_HIDING (Super_Novice): cast as Thief
TF_POISON (Super_Novice): cast as Thief
TF_DETOXIFY (Super_Novice): cast as Thief
ALL_BUYING_STORE (Super_Novice): cast as Merchant
"""
from skill_sweep import Sweep


# The shortcut bar, slot by slot (seedSkillSweep in server/paritysrv_test.go seeds the same).
HOTBAR = [500, 501, 502, 503, 504, 505, 506, 507, 508, 512, 513, 514, 515, 516, 517, 518, 519, 520, 521, 523,
          524, 525, 526, 527, 528, 529, 530, 531, 532, 534, 536, 537, 539, 540, 541, 542, 543, 535, 538, 544]

sw = Sweep('parity69', HOTBAR)

# Gunslinger
sw.job(24, 'gunslinger', setup=['@parityequip 13100', '@item 13200 300', '@item 13203 300'])
sw.cast(500, 'gs_glittering', 'self')
sw.say('@spiritball 5', then=30)
sw.cast(501, 'gs_fling', 'enemy')
sw.say('@parityequip 13200', then=30)
sw.say('@spiritball 5', then=30)
sw.cast(502, 'gs_tripleaction', 'enemy')
sw.say('@spiritball 5', then=30)
sw.cast(503, 'gs_bullseye', 'enemy', cast_ms=500)
sw.say('@spiritball 5', then=30)
sw.cast(504, 'gs_madnesscancel', 'self', cast_ms=3000, after_ms=4000)
sw.say('@spiritball 5', then=30)
sw.cast(505, 'gs_adjustment', 'self', cast_ms=1000, after_ms=1000)
sw.say('@spiritball 5', then=30)
sw.cast(506, 'gs_increasing', 'self', after_ms=1000)
sw.say('@spiritball 5', then=30)
sw.cast(507, 'gs_magicalbullet', 'enemy')
sw.say('@spiritball 5', then=30)
sw.cast(508, 'gs_cracker', 'enemy', after_ms=1000)
sw.cast(512, 'gs_tracking', 'enemy', cast_ms=3000)
sw.cast(513, 'gs_disarm', 'enemy')
sw.cast(514, 'gs_piercingshot', 'enemy', cast_ms=1500)
sw.cast(515, 'gs_rapidshower', 'enemy', after_ms=1000)
sw.cast(516, 'gs_desperado', 'self', after_ms=1000)
sw.say('@parityequip 13157', then=30)
sw.cast(517, 'gs_gatlingfever', 'self')
sw.cast(517, 'gs_gatlingfever_off', 'self')
sw.say('@parityequip 13154', then=30)
sw.cast(518, 'gs_dust', 'enemy', cast_ms=1000, after_ms=1000)
sw.cast(519, 'gs_fullbuster', 'enemy', after_ms=3000)
sw.cast(520, 'gs_spreadattack', 'enemy')
sw.say('@parityequip 13160', then=30)
sw.say('@parityequip 13203', then=30)
sw.cast(521, 'gs_grounddrift', 'ground', cast_ms=2000)
sw.leave_and_return('units')

# Ninja
sw.job(25, 'ninja', setup=['@parityequip 1201', '@item 13250 300', '@item 13255 300', '@item 7524 3', '@item 7521 3', '@item 7522 3', '@item 7523 3'])
sw.say('@parityequip 13250', then=30)
sw.cast(523, 'nj_syuriken', 'enemy')
sw.say('@parityequip 13255', then=30)
sw.cast(524, 'nj_kunai', 'enemy', after_ms=1000)
sw.say('@parityequip 13301', then=30)
sw.cast(525, 'nj_huuma', 'enemy', cast_ms=3000, after_ms=2000)
sw.cast(526, 'nj_zenynage', 'enemy', after_ms=5000)
sw.cast(527, 'nj_tatamigaeshi', 'self', after_ms=3000)
sw.cast(528, 'nj_kasumikiri', 'enemy', after_ms=1000)
sw.cast(529, 'nj_shadowjump', 'ground', after_ms=1000)
sw.cast(530, 'nj_kirikage', 'enemy')
sw.cast(531, 'nj_utsusemi', 'self', after_ms=1000)
sw.cast(532, 'nj_bunsinjyutsu', 'self', cast_ms=1000, after_ms=1000)
sw.cast(534, 'nj_kouenka', 'enemy', cast_ms=7000)
sw.cast(536, 'nj_bakuenryu', 'enemy', cast_ms=3000, after_ms=2000)
sw.cast(537, 'nj_hyousensou', 'enemy', cast_ms=7000)
sw.cast(539, 'nj_hyousyouraku', 'self', cast_ms=4000, after_ms=2000)
sw.cast(540, 'nj_huujin', 'enemy', cast_ms=6000, after_ms=1000)
sw.cast(541, 'nj_raigekisai', 'self', cast_ms=4000)
sw.cast(542, 'nj_kamaitachi', 'enemy', cast_ms=4000)
sw.cast(543, 'nj_nen', 'self', cast_ms=1000)
sw.cast(535, 'nj_kaensin', 'self', cast_ms=1500, after_ms=1000)
sw.cast(538, 'nj_suiton', 'ground', cast_ms=3000)
sw.leave_and_return('units')
sw.cast(544, 'nj_issen', 'enemy')
sw.say('@heal', then=30)

# Super_Novice
sw.job(23, 'super_novice')
STEPS = sw.end()
