"""The skill sweep for the Thief line (parity67): Thief, Assassin, Rogue, Assassin Cross and Stalker;
Hiding, Cloaking and Chase Walk started and ended, Grimtooth and Raid from hiding, Venom Dust and
the graffiti left and come back to.

Cast here, by class:
Thief (6): TF_STEAL, TF_HIDING, TF_POISON, TF_DETOXIFY, TF_SPRINKLESAND, TF_PICKSTONE, TF_THROWSTONE,
    TF_BACKSLIDING
Assassin (12): AS_CLOAKING, AS_SONICBLOW, AS_GRIMTOOTH, AS_ENCHANTPOISON, AS_POISONREACT, AS_SPLASHER,
    AS_VENOMKNIFE, AS_VENOMDUST
Rogue (17): RG_GRAFFITI, RG_STEALCOIN, RG_BACKSTAP, RG_RAID, RG_STRIPWEAPON, RG_STRIPSHIELD, RG_STRIPARMOR,
    RG_STRIPHELM, RG_FLAGGRAFFITI, RG_CLOSECONFINE, RG_CLEANER, RG_INTIMIDATE
Assassin_Cross (4013): ASC_EDP, ASC_BREAKER, ASC_METEORASSAULT, ASC_CDP
Stalker (4018): ST_CHASEWALK, ST_REJECTSWORD, ST_PRESERVE, ST_FULLSTRIP
Not cast here:
AC_DOUBLE (Rogue): cast as Archer
HT_REMOVETRAP (Rogue): cast as Hunter
"""
from skill_sweep import Sweep


# The shortcut bar, slot by slot (seedSkillSweep in server/paritysrv_test.go seeds the same).
HOTBAR = [50, 51, 52, 53, 149, 151, 152, 150, 135, 136, 137, 138, 139, 141, 1004, 140, 220, 211, 212, 214,
          215, 216, 217, 218, 221, 1005, 222, 219, 378, 379, 406, 407, 389, 390, 475, 476]

sw = Sweep('parity67', HOTBAR)

# Thief
sw.job(6, 'thief', setup=['@parityequip 1201', '@item 7049 3'])
sw.cast(50, 'tf_steal', 'enemy')
sw.cast(51, 'tf_hiding', 'self')
sw.cast(51, 'tf_hiding_off', 'self')
sw.cast(52, 'tf_poison', 'enemy')
sw.cast(53, 'tf_detoxify', 'friend')
sw.cast(149, 'tf_sprinklesand', 'enemy')
sw.cast(151, 'tf_pickstone', 'self', cast_ms=500)
sw.cast(152, 'tf_throwstone', 'enemy')
sw.cast(150, 'tf_backsliding', 'self')
sw.back_to_spawn()

# Assassin
sw.job(12, 'assassin', setup=['@parityequip 1252', '@item 716 3', '@item 1771 300'])
sw.cast(135, 'as_cloaking', 'self')
sw.cast(135, 'as_cloaking_off', 'self')
sw.cast(136, 'as_sonicblow', 'enemy', after_ms=2000)
sw.cast(51, 'hiding', 'self')
sw.cast(137, 'as_grimtooth', 'enemy')
sw.cast(51, 'unhiding', 'self')
sw.cast(138, 'as_enchantpoison', 'friend')
sw.cast(139, 'as_poisonreact', 'self')
sw.cast(141, 'as_splasher', 'enemy', cast_ms=1000)
sw.say('@parityequip 1771', then=30)
sw.cast(1004, 'as_venomknife', 'enemy')
sw.cast(140, 'as_venomdust', 'ground', near=True)
sw.leave_and_return('units')

# Rogue
sw.job(17, 'rogue', setup=['@parityequip 1201', '@item 716 3'])
sw.cast(220, 'rg_graffiti', 'ground', near=True)
sw.s.text('parity', then=4).key('enter', then=20)
sw.leave_and_return('units')
sw.cast(211, 'rg_stealcoin', 'enemy')
sw.cast(212, 'rg_backstap', 'enemy', after_ms=500)
sw.cast(51, 'hiding', 'self')
sw.cast(214, 'rg_raid', 'self')
sw.cast(215, 'rg_stripweapon', 'enemy', cast_ms=1000, after_ms=1000)
sw.cast(216, 'rg_stripshield', 'enemy', cast_ms=1000, after_ms=1000)
sw.cast(217, 'rg_striparmor', 'enemy', cast_ms=1000, after_ms=1000)
sw.cast(218, 'rg_striphelm', 'enemy', cast_ms=1000, after_ms=1000)
sw.cast(221, 'rg_flaggraffiti', 'ground', near=True)
sw.cast(1005, 'rg_closeconfine', 'enemy')
sw.cast(222, 'rg_cleaner', 'ground', near=True, at=(434, 381))
sw.cast(219, 'rg_intimidate', 'enemy')
sw.back_to_spawn()

# Assassin_Cross
sw.job(4013, 'assassin_cross', setup=['@parityequip 1252', '@item 678 3'])
sw.cast(378, 'asc_edp', 'self', after_ms=2000)
sw.cast(379, 'asc_breaker', 'enemy', cast_ms=500, after_ms=2800)
sw.cast(406, 'asc_meteorassault', 'self', cast_ms=500, after_ms=500)
sw.cast(407, 'asc_cdp', 'self', after_ms=500)

# Stalker
sw.job(4018, 'stalker', setup=['@parityequip 1201'])
sw.cast(389, 'st_chasewalk', 'self', cast_ms=1200)
sw.cast(389, 'st_chasewalk_off', 'self', cast_ms=1200)
sw.cast(390, 'st_rejectsword', 'self')
sw.cast(475, 'st_preserve', 'self', cast_ms=1000)
sw.cast(476, 'st_fullstrip', 'enemy', after_ms=1000)
STEPS = sw.end()
