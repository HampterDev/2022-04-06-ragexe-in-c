"""The skill sweep for the Swordman line (parity61): Swordman, Knight, Crusader, Lord Knight and
Paladin, every skill each class adds cast from the shortcut bar at the Arena dummy, the character
or a cell; Endure, Two-Hand Quicken and the Lord Knight's buffs ended by a death, the Crusader's
toggles and Gospel started and ended.

Cast here, by class:
Swordsman (1): SM_BASH, SM_PROVOKE, SM_MAGNUM, SM_AUTOBERSERK, SM_ENDURE
Knight (7): KN_PIERCE, KN_SPEARSTAB, KN_SPEARBOOMERANG, KN_BOWLINGBASH, KN_AUTOCOUNTER, KN_CHARGEATK,
    KN_BRANDISHSPEAR, KN_TWOHANDQUICKEN
Crusader (14): AL_CURE, AL_HEAL, CR_AUTOGUARD, CR_SHIELDCHARGE, CR_SHIELDBOOMERANG, CR_REFLECTSHIELD,
    CR_HOLYCROSS, CR_GRANDCROSS, CR_DEVOTION, CR_PROVIDENCE, CR_DEFENDER, CR_SHRINK, CR_SPEARQUICKEN
Lord_Knight (4008): LK_AURABLADE, LK_CONCENTRATION, LK_SPIRALPIERCE, LK_HEADCRUSH, LK_JOINTBEAT, LK_PARRYING,
    LK_TENSIONRELAX
Paladin (4015): PA_PRESSURE, PA_SACRIFICE, PA_SHIELDCHAIN, PA_GOSPEL
Not cast here:
KN_ONEHAND (Knight): @allskill does not put it in the skill list the client is sent, so its slot stays empty
LK_BERSERK (Lord_Knight): @allskill does not put it in the skill list the client is sent, so its slot stays
    empty
"""
from skill_sweep import Sweep


# The shortcut bar, slot by slot (seedSkillSweep in server/paritysrv_test.go seeds the same).
HOTBAR = [5, 6, 7, 146, 8, 56, 58, 59, 62, 61, 1001, 57, 60, 35, 28, 249, 250, 251, 252, 253, 254, 255, 256,
          257, 1002, 258, 355, 357, 397, 398, 399, 356, 358, 367, 368, 480, 369]

sw = Sweep('parity61', HOTBAR)

# Swordsman
sw.job(1, 'swordsman', setup=['@parityequip 1101'])
sw.cast(5, 'sm_bash', 'enemy')
sw.cast(6, 'sm_provoke', 'enemy')
sw.cast(7, 'sm_magnum', 'self', after_ms=2000)
sw.cast(146, 'sm_autoberserk', 'self')
sw.cast(146, 'sm_autoberserk_off', 'self')
sw.cast(8, 'sm_endure', 'self')
sw.die_and_revive('death')

# Knight
sw.job(7, 'knight', setup=['@parityequip 1410'])
sw.cast(56, 'kn_pierce', 'enemy')
sw.cast(58, 'kn_spearstab', 'enemy')
sw.cast(59, 'kn_spearboomerang', 'enemy', after_ms=1000)
sw.cast(62, 'kn_bowlingbash', 'enemy', cast_ms=700)
sw.cast(61, 'kn_autocounter', 'self')
sw.cast(1001, 'kn_chargeatk', 'enemy', cast_ms=300, after_ms=300)
sw.back_to_spawn()
sw.say('@mount', then=30)
sw.cast(57, 'kn_brandishspear', 'enemy', cast_ms=700)
sw.say('@parityequip 1116', then=30)
sw.cast(60, 'kn_twohandquicken', 'self')
sw.die_and_revive('death')

# Crusader
sw.job(14, 'crusader', setup=['@parityequip 1401', '@parityequip 2101'])
sw.cast(35, 'al_cure', 'friend', after_ms=1000)
sw.cast(28, 'al_heal', 'friend', after_ms=1000)
sw.cast(249, 'cr_autoguard', 'self')
sw.cast(249, 'cr_autoguard_off', 'self')
sw.cast(250, 'cr_shieldcharge', 'enemy')
sw.cast(251, 'cr_shieldboomerang', 'enemy', after_ms=700)
sw.cast(252, 'cr_reflectshield', 'self')
sw.cast(253, 'cr_holycross', 'enemy')
sw.cast(254, 'cr_grandcross', 'self', cast_ms=2000, after_ms=1500)
sw.cast(255, 'cr_devotion', 'friend', cast_ms=3000)
sw.cast(256, 'cr_providence', 'friend', cast_ms=3000)
sw.cast(257, 'cr_defender', 'self', after_ms=800)
sw.cast(257, 'cr_defender_off', 'self', after_ms=800)
sw.cast(1002, 'cr_shrink', 'self')
sw.cast(1002, 'cr_shrink_off', 'self')
sw.say('@parityequip 1410', then=30)
sw.cast(258, 'cr_spearquicken', 'self')

# Lord_Knight
sw.job(4008, 'lord_knight', setup=['@parityequip 1410'])
sw.cast(355, 'lk_aurablade', 'self')
sw.cast(357, 'lk_concentration', 'self')
sw.cast(397, 'lk_spiralpierce', 'enemy', cast_ms=1000, after_ms=2000)
sw.cast(398, 'lk_headcrush', 'enemy', after_ms=500)
sw.cast(399, 'lk_jointbeat', 'enemy', after_ms=1000)
sw.say('@parityequip 1116', then=30)
sw.cast(356, 'lk_parrying', 'self')
sw.cast(358, 'lk_tensionrelax', 'self')
sw.s.key('insert', then=20)
sw.die_and_revive('death')

# Paladin
sw.job(4015, 'paladin', setup=['@parityequip 1401', '@parityequip 2101'])
sw.cast(367, 'pa_pressure', 'enemy', cast_ms=4000, after_ms=4000)
sw.cast(368, 'pa_sacrifice', 'self')
sw.cast(480, 'pa_shieldchain', 'enemy', cast_ms=1000, after_ms=1000)
sw.cast(369, 'pa_gospel', 'self')
sw.cast(369, 'pa_gospel_off', 'self')
sw.leave_and_return('units')
STEPS = sw.end()
