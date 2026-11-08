"""The skill sweep for the Archer line (parity63): Archer, Hunter, Bard, Sniper and Clown with a bow
or an instrument and arrows; the Hunter's traps set, sprung and removed, left and come back to
and warped away from; each song performed in turn and walked out of view and back with.

Cast here, by class:
Archer (3): AC_CONCENTRATION, AC_DOUBLE, AC_SHOWER, AC_MAKINGARROW, AC_CHARGEARROW
Hunter (11): HT_BLITZBEAT, HT_PHANTASMIC, HT_POWER, HT_SKIDTRAP, HT_LANDMINE, HT_ANKLESNARE, HT_SHOCKWAVE,
    HT_SANDMAN, HT_FLASHER, HT_FREEZINGTRAP, HT_BLASTMINE, HT_CLAYMORETRAP, HT_TALKIEBOX, HT_DETECTING,
    HT_SPRINGTRAP, HT_REMOVETRAP
Bard (19): BA_MUSICALSTRIKE, BA_FROSTJOKE, BA_PANGVOICE, BA_DISSONANCE, BA_WHISTLE, BA_ASSASSINCROSS,
    BA_POEMBRAGI, BA_APPLEIDUN, BD_LULLABY, BD_RICHMANKIM, BD_ETERNALCHAOS, BD_DRUMBATTLEFIELD,
    BD_RINGNIBELUNGEN, BD_ROKISWEIL, BD_INTOABYSS, BD_SIEGFRIED, BD_ADAPTATION, BD_ENCORE
Sniper (4012): SN_SIGHT, SN_FALCONASSAULT, SN_SHARPSHOOTING, SN_WINDWALK
Clown (4020): CG_ARROWVULCAN, CG_MARIONETTE, CG_LONGINGFREEDOM, CG_TAROTCARD, CG_MOONLIT, CG_HERMODE
"""
from skill_sweep import Sweep


# The shortcut bar, slot by slot (seedSkillSweep in server/paritysrv_test.go seeds the same).
HOTBAR = [45, 46, 47, 147, 148, 129, 1009, 499, 115, 116, 117, 118, 119, 120, 121, 122, 123, 125, 130, 131,
          124, 316, 318, 1010, 317, 319, 320, 321, 322, 306, 307, 308, 309, 310, 311, 312, 313, 304, 305, 380,
          381, 382, 383, 394, 396, 487, 489, 395, 488]

sw = Sweep('parity63', HOTBAR)

# Archer
sw.job(3, 'archer', setup=['@parityequip 1701', '@item 1750 300'])
sw.cast(45, 'ac_concentration', 'self')
sw.say('@parityequip 1750', then=30)
sw.cast(46, 'ac_double', 'enemy')
sw.cast(47, 'ac_shower', 'ground')
sw.cast(147, 'ac_makingarrow', 'self')
sw.s.click_ui('UIMakingArrowListWnd/UIBitmapButton#177', then=20)
sw.cast(148, 'ac_chargearrow', 'enemy', cast_ms=1500)

# Hunter
sw.job(11, 'hunter', setup=['@falcon', '@parityequip 1701', '@item 1750 300', '@item 1065 6'])
sw.cast(129, 'ht_blitzbeat', 'enemy', cast_ms=1500, after_ms=1000)
sw.cast(1009, 'ht_phantasmic', 'enemy')
sw.say('@parityequip 1750', then=30)
sw.cast(499, 'ht_power', 'enemy')
sw.cast(115, 'ht_skidtrap', 'ground', near=True)
sw.cast(116, 'ht_landmine', 'ground', near=True)
sw.cast(117, 'ht_anklesnare', 'ground', near=True)
sw.cast(118, 'ht_shockwave', 'ground', near=True)
sw.cast(119, 'ht_sandman', 'ground', near=True)
sw.cast(120, 'ht_flasher', 'ground', near=True)
sw.cast(121, 'ht_freezingtrap', 'ground', near=True)
sw.cast(122, 'ht_blastmine', 'ground', near=True)
sw.cast(123, 'ht_claymoretrap', 'ground', near=True)
sw.cast(125, 'ht_talkiebox', 'ground', near=True)
sw.s.text('parity', then=4).key('enter', then=20)
sw.leave_and_return('units')
sw.cast(130, 'ht_detecting', 'ground')
sw.cast(131, 'ht_springtrap', 'trap')
sw.cast(124, 'ht_removetrap', 'trap')
sw.warp_away_and_back('warp')

# Bard
sw.job(19, 'bard', setup=['@parityequip 1901', '@item 1750 300'])
sw.say('@parityequip 1750', then=30)
sw.cast(316, 'ba_musicalstrike', 'enemy', cast_ms=1500)
sw.cast(318, 'ba_frostjoke', 'self', after_ms=3000)
sw.cast(1010, 'ba_pangvoice', 'enemy', cast_ms=1000, after_ms=2000)
sw.cast(317, 'ba_dissonance', 'self')
sw.cast(319, 'ba_whistle', 'self')
sw.cast(320, 'ba_assassincross', 'self')
sw.cast(321, 'ba_poembragi', 'self')
sw.cast(322, 'ba_appleidun', 'self')
sw.leave_and_return('units')
sw.cast(306, 'bd_lullaby', 'self')
sw.cast(307, 'bd_richmankim', 'self')
sw.cast(308, 'bd_eternalchaos', 'self')
sw.cast(309, 'bd_drumbattlefield', 'self')
sw.cast(310, 'bd_ringnibelungen', 'self')
sw.cast(311, 'bd_rokisweil', 'self')
sw.cast(312, 'bd_intoabyss', 'self')
sw.cast(313, 'bd_siegfried', 'self')
sw.cast(304, 'bd_adaptation', 'self')
sw.cast(305, 'bd_encore', 'self')
sw.leave_and_return('units')

# Sniper
sw.job(4012, 'sniper', setup=['@falcon', '@parityequip 1701', '@item 1750 300'])
sw.cast(380, 'sn_sight', 'self')
sw.cast(381, 'sn_falconassault', 'enemy', cast_ms=1000, after_ms=3000)
sw.say('@parityequip 1750', then=30)
sw.cast(382, 'sn_sharpshooting', 'enemy', cast_ms=2000, after_ms=1500)
sw.cast(383, 'sn_windwalk', 'self', cast_ms=5600, after_ms=2000)

# Clown
sw.job(4020, 'clown', setup=['@parityequip 1901', '@item 1750 300'])
sw.say('@parityequip 1750', then=30)
sw.cast(394, 'cg_arrowvulcan', 'enemy', cast_ms=3800, after_ms=3000)
sw.cast(396, 'cg_marionette', 'friend')
sw.cast(487, 'cg_longingfreedom', 'self')
sw.cast(489, 'cg_tarotcard', 'enemy', cast_ms=1000, after_ms=3000)
sw.cast(395, 'cg_moonlit', 'self')
sw.cast(488, 'cg_hermode', 'self')
sw.leave_and_return('units')
STEPS = sw.end()
