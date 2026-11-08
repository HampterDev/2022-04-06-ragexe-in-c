"""The skill sweep for the Novice and the Taekwon line (parity68): Novice, Taekwon, Star Gladiator and
Soul Linker; Play Dead started and ended, a novice's death, the stances, Feeling's and Hatred's
questions, and Kaizel bringing the Soul Linker back from a death.

Cast here, by class:
Novice (0): NV_FIRSTAID, NV_TRICKDEAD
Taekwon (4046): TK_READYSTORM, TK_STORMKICK, TK_READYDOWN, TK_DOWNKICK, TK_READYTURN, TK_TURNKICK,
    TK_READYCOUNTER, TK_COUNTER, TK_DODGE, TK_JUMPKICK, TK_SEVENWIND, TK_MISSION, TK_HIGHJUMP, TK_RUN
Star_Gladiator (4047): SG_FEEL, SG_HATE, SG_SUN_WARM, SG_MOON_WARM, SG_STAR_WARM, SG_SUN_COMFORT,
    SG_MOON_COMFORT, SG_STAR_COMFORT, SG_FUSION
Soul_Linker (4049): SL_ALCHEMIST, SL_MONK, SL_STAR, SL_SAGE, SL_CRUSADER, SL_SUPERNOVICE, SL_KNIGHT,
    SL_WIZARD, SL_PRIEST, SL_BARDDANCER, SL_ROGUE, SL_ASSASIN, SL_BLACKSMITH, SL_HUNTER, SL_SOULLINKER,
    SL_KAAHI, SL_KAUPE, SL_KAITE, SL_STIN, SL_STUN, SL_SMA, SL_SWOO, SL_SKE, SL_SKA, SL_HIGH, SL_KAIZEL
Not cast here:
WE_BABY (Novice): needs a spouse or an adopted child
WE_CALLPARENT (Novice): needs a spouse or an adopted child
WE_CALLBABY (Novice): needs a spouse or an adopted child
"""
from skill_sweep import Sweep


# The shortcut bar, slot by slot (seedSkillSweep in server/paritysrv_test.go seeds the same).
HOTBAR = [142, 143, 412, 413, 414, 415, 416, 417, 418, 419, 420, 421, 425, 493, 426, 411, 427, 434, 428, 429,
          430, 431, 432, 433, 444, 445, 447, 448, 449, 450, 451, 452, 453, 454, 455, 456, 457, 458, 460, 461,
          463, 464, 465, 467, 468, 469, 470, 471, 472, 494, 462]

sw = Sweep('parity68', HOTBAR)

# Novice
sw.job(0, 'novice')
sw.cast(142, 'nv_firstaid', 'self')
sw.cast(143, 'nv_trickdead', 'self')
sw.cast(143, 'nv_trickdead_off', 'self')
sw.die_and_revive('death')

# Taekwon
sw.job(4046, 'taekwon')
sw.cast(412, 'tk_readystorm', 'self')
sw.cast(413, 'tk_stormkick', 'self')
sw.cast(414, 'tk_readydown', 'self')
sw.cast(415, 'tk_downkick', 'self')
sw.cast(416, 'tk_readyturn', 'self')
sw.cast(417, 'tk_turnkick', 'self')
sw.cast(418, 'tk_readycounter', 'self')
sw.cast(419, 'tk_counter', 'self')
sw.cast(420, 'tk_dodge', 'self')
sw.cast(421, 'tk_jumpkick', 'enemy')
sw.cast(425, 'tk_sevenwind', 'self')
sw.cast(493, 'tk_mission', 'self', cast_ms=1000)
sw.cast(426, 'tk_highjump', 'self', cast_ms=1000)
sw.back_to_spawn()
sw.cast(411, 'tk_run', 'self')
sw.cast(411, 'tk_run_off', 'self')
sw.back_to_spawn()

# Star_Gladiator
sw.job(4047, 'star_gladiator')
sw.cast(427, 'sg_feel', 'self', cast_ms=1000)
sw.s.click_ui('UIStarPlaceAcceptWnd/UIBitmapButton#176', then=20)
sw.cast(434, 'sg_hate', 'enemy', cast_ms=1000)
sw.cast(428, 'sg_sun_warm', 'self', after_ms=1000)
sw.cast(429, 'sg_moon_warm', 'self', after_ms=1000)
sw.cast(430, 'sg_star_warm', 'self', after_ms=1000)
sw.cast(431, 'sg_sun_comfort', 'self', after_ms=1000)
sw.cast(432, 'sg_moon_comfort', 'self', after_ms=1000)
sw.cast(433, 'sg_star_comfort', 'self', after_ms=1000)
sw.cast(444, 'sg_fusion', 'self', after_ms=1000)

# Soul_Linker
sw.job(4049, 'soul_linker')
sw.cast(445, 'sl_alchemist', 'friend', cast_ms=1000, after_ms=500)
sw.cast(447, 'sl_monk', 'friend', cast_ms=1000, after_ms=500)
sw.cast(448, 'sl_star', 'friend', cast_ms=1000, after_ms=500)
sw.cast(449, 'sl_sage', 'friend', cast_ms=1000, after_ms=500)
sw.cast(450, 'sl_crusader', 'friend', cast_ms=1000, after_ms=500)
sw.cast(451, 'sl_supernovice', 'friend', cast_ms=1000, after_ms=500)
sw.cast(452, 'sl_knight', 'friend', cast_ms=1000, after_ms=500)
sw.cast(453, 'sl_wizard', 'friend', cast_ms=1000, after_ms=500)
sw.cast(454, 'sl_priest', 'friend', cast_ms=1000, after_ms=500)
sw.cast(455, 'sl_barddancer', 'friend', cast_ms=1000, after_ms=500)
sw.cast(456, 'sl_rogue', 'friend', cast_ms=1000, after_ms=500)
sw.cast(457, 'sl_assasin', 'friend', cast_ms=1000, after_ms=500)
sw.cast(458, 'sl_blacksmith', 'friend', cast_ms=1000, after_ms=500)
sw.cast(460, 'sl_hunter', 'friend', cast_ms=1000, after_ms=500)
sw.cast(461, 'sl_soullinker', 'friend', cast_ms=1000, after_ms=500)
sw.cast(463, 'sl_kaahi', 'friend', after_ms=500)
sw.cast(464, 'sl_kaupe', 'friend', cast_ms=500, after_ms=500)
sw.cast(465, 'sl_kaite', 'friend', cast_ms=3000)
sw.cast(467, 'sl_stin', 'enemy', cast_ms=100, after_ms=500)
sw.cast(468, 'sl_stun', 'enemy', cast_ms=100, after_ms=500)
sw.cast(469, 'sl_sma', 'enemy', cast_ms=2000, after_ms=500)
sw.cast(470, 'sl_swoo', 'enemy', cast_ms=1000, after_ms=500)
sw.cast(471, 'sl_ske', 'enemy', cast_ms=1000, after_ms=500)
sw.cast(472, 'sl_ska', 'enemy', cast_ms=1000, after_ms=500)
sw.cast(494, 'sl_high', 'friend', cast_ms=1000, after_ms=500)
sw.cast(462, 'sl_kaizel', 'friend', cast_ms=1500)
sw.say('@die', then=150)
sw.s.snap('soul_linker_kaizel_revived')
STEPS = sw.end()
