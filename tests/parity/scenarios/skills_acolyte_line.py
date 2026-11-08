"""The skill sweep for the Acolyte line (parity65): Acolyte, Priest, Monk, High Priest and Champion;
a teleport and a warp portal, Pneuma, Sanctuary, Magnus and Basilica left and come back to, the
blessings ended by a death, spirit spheres and Fury before the skills that need them, Asura
Strike, and Steel Body ended by a death.

Cast here, by class:
Acolyte (4): AL_RUWACH, AL_INCAGI, AL_DECAGI, AL_HOLYWATER, AL_CRUCIS, AL_ANGELUS, AL_BLESSING, AL_HOLYLIGHT,
    AL_PNEUMA, AL_WARP, AL_TELEPORT
Priest (8): ALL_RESURRECTION, PR_IMPOSITIO, PR_SUFFRAGIUM, PR_ASPERSIO, PR_BENEDICTIO, PR_SLOWPOISON,
    PR_STRECOVERY, PR_KYRIE, PR_MAGNIFICAT, PR_GLORIA, PR_LEXDIVINA, PR_TURNUNDEAD, PR_LEXAETERNA,
    PR_REDEMPTIO, PR_SANCTUARY, PR_MAGNUS
Monk (15): MO_CALLSPIRITS, MO_ABSORBSPIRITS, MO_INVESTIGATE, MO_FINGEROFFENSIVE, MO_BLADESTOP, MO_CHAINCOMBO,
    MO_COMBOFINISH, MO_KITRANSLATION, MO_BALKYOUNG, MO_EXPLOSIONSPIRITS, MO_BODYRELOCATION, MO_EXTREMITYFIST,
    MO_STEELBODY
High_Priest (4009): HP_ASSUMPTIO, HP_BASILICA
Champion (4016): CH_PALMSTRIKE, CH_TIGERFIST, CH_CHAINCRUSH, CH_SOULCOLLECT
Not cast here:
AL_HEAL (Acolyte): cast as Crusader
AL_CURE (Acolyte): cast as Crusader
MG_SAFETYWALL (Priest): cast as Magician
"""
from skill_sweep import Sweep


# The shortcut bar, slot by slot (seedSkillSweep in server/paritysrv_test.go seeds the same).
HOTBAR = [24, 29, 30, 31, 32, 33, 34, 156, 25, 27, 26, 54, 66, 67, 68, 69, 71, 72, 73, 74, 75, 76, 77, 78,
          1014, 70, 79, 261, 262, 266, 267, 269, 272, 273, 1015, 1016, 270, 264, 271, 268, 361, 362, 370, 371,
          372, 401]

sw = Sweep('parity65', HOTBAR)

# Acolyte
sw.job(4, 'acolyte', setup=['@parityequip 1504', '@item 717 3'])
sw.cast(24, 'al_ruwach', 'self')
sw.cast(29, 'al_incagi', 'friend', cast_ms=1000, after_ms=1000)
sw.cast(30, 'al_decagi', 'enemy', cast_ms=1000, after_ms=1000)
sw.cast(31, 'al_holywater', 'self', cast_ms=1000, after_ms=500)
sw.cast(32, 'al_crucis', 'self', cast_ms=500, after_ms=2000)
sw.cast(33, 'al_angelus', 'self', cast_ms=500, after_ms=3500)
sw.cast(34, 'al_blessing', 'friend')
sw.cast(156, 'al_holylight', 'enemy', cast_ms=2000)
sw.cast(25, 'al_pneuma', 'ground')
sw.cast(27, 'al_warp', 'ground', cast_ms=1000)
sw.s.converse('acolyte_warp_menu', choices=(0,), rounds=1, per=40)
sw.leave_and_return('units')
sw.cast(26, 'al_teleport', 'self')
sw.back_to_spawn()

# Priest
sw.job(8, 'priest', setup=['@parityequip 1504', '@item 717 3', '@item 523 3'])
sw.cast(54, 'all_resurrection', 'friend', after_ms=3000)
sw.cast(66, 'pr_impositio', 'friend', after_ms=3000)
sw.cast(67, 'pr_suffragium', 'friend', after_ms=2000)
sw.cast(68, 'pr_aspersio', 'friend', after_ms=2000)
sw.cast(69, 'pr_benedictio', 'ground')
sw.cast(71, 'pr_slowpoison', 'friend')
sw.cast(72, 'pr_strecovery', 'friend', after_ms=2000)
sw.cast(73, 'pr_kyrie', 'friend', cast_ms=2000, after_ms=2000)
sw.cast(74, 'pr_magnificat', 'self', cast_ms=4000, after_ms=2000)
sw.cast(75, 'pr_gloria', 'self', after_ms=2000)
sw.cast(76, 'pr_lexdivina', 'enemy', after_ms=3000)
sw.cast(77, 'pr_turnundead', 'enemy', cast_ms=1000, after_ms=3000)
sw.cast(78, 'pr_lexaeterna', 'enemy', after_ms=3000)
sw.cast(1014, 'pr_redemptio', 'self', cast_ms=4000)
sw.cast(70, 'pr_sanctuary', 'ground', cast_ms=5000)
sw.cast(79, 'pr_magnus', 'ground', cast_ms=15000, after_ms=4000)
sw.leave_and_return('units')
sw.die_and_revive('death')

# Monk
sw.job(15, 'monk', setup=['@parityequip 1801'])
sw.cast(261, 'mo_callspirits', 'self', cast_ms=1000)
sw.cast(262, 'mo_absorbspirits', 'friend', cast_ms=2000)
sw.say('@spiritball 5', then=30)
sw.cast(266, 'mo_investigate', 'enemy', cast_ms=1000, after_ms=500)
sw.say('@spiritball 5', then=30)
sw.cast(267, 'mo_fingeroffensive', 'enemy', cast_ms=1000, after_ms=800)
sw.say('@spiritball 5', then=30)
sw.cast(269, 'mo_bladestop', 'self')
sw.cast(272, 'mo_chaincombo', 'self', after_ms=1000)
sw.say('@spiritball 5', then=30)
sw.cast(273, 'mo_combofinish', 'self', after_ms=700)
sw.say('@spiritball 5', then=30)
sw.cast(1015, 'mo_kitranslation', 'friend', cast_ms=2000, after_ms=1000)
sw.cast(1016, 'mo_balkyoung', 'enemy', after_ms=2000)
sw.say('@spiritball 5', then=30)
sw.cast(270, 'mo_explosionspirits', 'self')
sw.say('@spiritball 5', then=30)
sw.cast(264, 'mo_bodyrelocation', 'ground')
sw.back_to_spawn()
sw.say('@spiritball 5', then=30)
sw.cast(271, 'mo_extremityfist', 'enemy', cast_ms=2000, after_ms=1000)
sw.say('@heal', then=30)
sw.say('@spiritball 5', then=30)
sw.cast(268, 'mo_steelbody', 'self', cast_ms=5000)
sw.die_and_revive('death')

# High_Priest
sw.job(4009, 'high_priest', setup=['@parityequip 1504', '@item 715 3', '@item 716 3', '@item 717 3', '@item 523 3'])
sw.cast(361, 'hp_assumptio', 'friend', cast_ms=3000, after_ms=1500)
sw.cast(362, 'hp_basilica', 'self', cast_ms=9000, after_ms=6000)
sw.cast(362, 'hp_basilica_off', 'self', cast_ms=9000, after_ms=6000)
sw.leave_and_return('units')

# Champion
sw.job(4016, 'champion', setup=['@parityequip 1801'])
sw.say('@spiritball 5', then=30)
sw.cast(270, 'fury', 'self')
sw.cast(370, 'ch_palmstrike', 'enemy', after_ms=300)
sw.say('@spiritball 5', then=30)
sw.cast(371, 'ch_tigerfist', 'self', after_ms=700)
sw.say('@spiritball 5', then=30)
sw.cast(372, 'ch_chaincrush', 'self', after_ms=1000)
sw.cast(401, 'ch_soulcollect', 'self', cast_ms=2000)
STEPS = sw.end()
