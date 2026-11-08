"""The skill sweep for the Magician line (parity62): Magician, Wizard, Sage, High Wizard and
Professor; the ground units (Safety Wall, Fire Wall, Fire Pillar, Ice Wall, Quagmire, the land
skills, Gravitation, Fog Wall) left and come back to, and a warp away and back while Land
Protector lasts.

Cast here, by class:
Magician (2): MG_SIGHT, MG_NAPALMBEAT, MG_SOULSTRIKE, MG_COLDBOLT, MG_FROSTDIVER, MG_STONECURSE, MG_FIREBALL,
    MG_FIREBOLT, MG_LIGHTNINGBOLT, MG_THUNDERSTORM, MG_ENERGYCOAT, MG_SAFETYWALL, MG_FIREWALL
Wizard (9): WZ_SIGHTRASHER, WZ_METEOR, WZ_JUPITEL, WZ_VERMILION, WZ_WATERBALL, WZ_FROSTNOVA, WZ_STORMGUST,
    WZ_EARTHSPIKE, WZ_HEAVENDRIVE, WZ_ESTIMATION, WZ_SIGHTBLASTER, WZ_FIREPILLAR, WZ_ICEWALL, WZ_QUAGMIRE
Sage (16): SA_CASTCANCEL, SA_MAGICROD, SA_SPELLBREAKER, SA_AUTOSPELL, SA_FLAMELAUNCHER, SA_FROSTWEAPON,
    SA_LIGHTNINGLOADER, SA_SEISMICWEAPON, SA_DISPELL, SA_CREATECON, SA_ELEMENTWATER, SA_ELEMENTGROUND,
    SA_ELEMENTFIRE, SA_ELEMENTWIND, SA_VOLCANO, SA_DELUGE, SA_VIOLENTGALE, SA_LANDPROTECTOR, SA_ABRACADABRA
High_Wizard (4010): HW_MAGICCRASHER, HW_MAGICPOWER, HW_NAPALMVULCAN, HW_GRAVITATION, HW_GANBANTEIN
Professor (4017): PF_HPCONVERSION, PF_SOULCHANGE, PF_SOULBURN, PF_MINDBREAKER, PF_MEMORIZE, PF_DOUBLECASTING,
    PF_FOGWALL, PF_SPIDERWEB
Not cast here:
WZ_ESTIMATION (Sage): cast as Wizard
WZ_EARTHSPIKE (Sage): cast as Wizard
WZ_HEAVENDRIVE (Sage): cast as Wizard
"""
from skill_sweep import Sweep


# The shortcut bar, slot by slot (seedSkillSweep in server/paritysrv_test.go seeds the same).
HOTBAR = [10, 11, 13, 14, 15, 16, 17, 19, 20, 21, 157, 12, 18, 81, 83, 84, 85, 86, 88, 89, 90, 91, 93, 1006,
          80, 87, 92, 275, 276, 277, 279, 280, 281, 282, 283, 289, 1007, 1008, 1017, 1018, 1019, 285, 286,
          287, 288, 290, 365, 366, 400, 484, 483, 373, 374, 375, 402, 403, 482, 404, 405]

sw = Sweep('parity62', HOTBAR)

# Magician
sw.job(2, 'magician', setup=['@parityequip 1601', '@item 716 3', '@item 717 3'])
sw.cast(10, 'mg_sight', 'self')
sw.cast(11, 'mg_napalmbeat', 'enemy', cast_ms=500, after_ms=1000)
sw.cast(13, 'mg_soulstrike', 'enemy', cast_ms=500, after_ms=1800)
sw.cast(14, 'mg_coldbolt', 'enemy', cast_ms=7000, after_ms=2800)
sw.cast(15, 'mg_frostdiver', 'enemy', cast_ms=800, after_ms=1500)
sw.cast(16, 'mg_stonecurse', 'enemy', cast_ms=1000)
sw.cast(17, 'mg_fireball', 'enemy', cast_ms=1000, after_ms=1000)
sw.cast(19, 'mg_firebolt', 'enemy', cast_ms=7000, after_ms=2800)
sw.cast(20, 'mg_lightningbolt', 'enemy', cast_ms=7000, after_ms=2800)
sw.cast(21, 'mg_thunderstorm', 'ground', cast_ms=10000, after_ms=2000)
sw.cast(157, 'mg_energycoat', 'self', cast_ms=5000)
sw.cast(12, 'mg_safetywall', 'ground', cast_ms=1000)
sw.cast(18, 'mg_firewall', 'ground', cast_ms=650)
sw.leave_and_return('units')

# Wizard
sw.job(9, 'wizard', setup=['@parityequip 1601', '@item 717 3'])
sw.cast(10, 'sight', 'self')
sw.cast(81, 'wz_sightrasher', 'self', cast_ms=500, after_ms=2000)
sw.cast(83, 'wz_meteor', 'ground', cast_ms=15000, after_ms=7000)
sw.cast(84, 'wz_jupitel', 'enemy', cast_ms=7000)
sw.cast(85, 'wz_vermilion', 'ground', cast_ms=10500, after_ms=5000)
sw.cast(86, 'wz_waterball', 'enemy', cast_ms=5000)
sw.cast(88, 'wz_frostnova', 'self', cast_ms=4000, after_ms=1000)
sw.cast(89, 'wz_stormgust', 'ground', cast_ms=15000, after_ms=5000)
sw.cast(90, 'wz_earthspike', 'enemy', cast_ms=5000, after_ms=700)
sw.cast(91, 'wz_heavendrive', 'ground', cast_ms=5000, after_ms=700)
sw.cast(93, 'wz_estimation', 'enemy')
sw.s.click_ui('UIMonsterInfoWnd/UIBitmapButton#176', then=20)
sw.cast(1006, 'wz_sightblaster', 'self', cast_ms=2000)
sw.cast(80, 'wz_firepillar', 'ground', cast_ms=300, after_ms=1000)
sw.cast(87, 'wz_icewall', 'ground')
sw.cast(92, 'wz_quagmire', 'ground', after_ms=1000)
sw.leave_and_return('units')

# Sage
sw.job(16, 'sage', setup=['@parityequip 1550', '@item 990 3', '@item 991 3', '@item 992 3', '@item 993 3', '@item 715 6', '@item 12115 3', '@item 12116 3', '@item 12114 3', '@item 12117 3', '@item 717 3'])
sw.cast(275, 'sa_castcancel', 'self')
sw.cast(276, 'sa_magicrod', 'self')
sw.cast(277, 'sa_spellbreaker', 'enemy', cast_ms=700)
sw.cast(279, 'sa_autospell', 'self', cast_ms=3000)
sw.s.click_ui('UISpellListWnd', at=(0.39, 0.23), then=10).click_ui('UISpellListWnd/UIBitmapButton#176', then=20)
sw.cast(280, 'sa_flamelauncher', 'friend', cast_ms=3000)
sw.cast(281, 'sa_frostweapon', 'friend', cast_ms=3000)
sw.cast(282, 'sa_lightningloader', 'friend', cast_ms=3000)
sw.cast(283, 'sa_seismicweapon', 'friend', cast_ms=3000)
sw.cast(289, 'sa_dispell', 'enemy', cast_ms=2000)
sw.cast(1007, 'sa_createcon', 'self')
sw.cast(1008, 'sa_elementwater', 'enemy', cast_ms=2000, after_ms=1000)
sw.cast(1017, 'sa_elementground', 'enemy', cast_ms=2000, after_ms=1000)
sw.cast(1018, 'sa_elementfire', 'enemy', cast_ms=2000, after_ms=1000)
sw.cast(1019, 'sa_elementwind', 'enemy', cast_ms=2000, after_ms=1000)
sw.cast(285, 'sa_volcano', 'ground', cast_ms=5000, near=True)
sw.cast(286, 'sa_deluge', 'ground', cast_ms=5000, near=True)
sw.cast(287, 'sa_violentgale', 'ground', cast_ms=5000, near=True)
sw.cast(288, 'sa_landprotector', 'ground', cast_ms=5000, near=True)
sw.leave_and_return('units')
sw.cast(290, 'sa_abracadabra', 'self')
sw.cancel()
sw.warp_away_and_back('warp')

# High_Wizard
sw.job(4010, 'high_wizard', setup=['@parityequip 1601', '@item 717 3', '@item 715 3'])
sw.cast(365, 'hw_magiccrasher', 'enemy', cast_ms=300, after_ms=300)
sw.cast(366, 'hw_magicpower', 'self', cast_ms=700)
sw.cast(400, 'hw_napalmvulcan', 'enemy', cast_ms=1000, after_ms=1000)
sw.cast(484, 'hw_gravitation', 'ground', cast_ms=5000, after_ms=9000)
sw.leave_and_return('units')
sw.cast(483, 'hw_ganbantein', 'ground', cast_ms=3000, after_ms=5000)

# Professor
sw.job(4017, 'professor', setup=['@parityequip 1550', '@item 1025 3'])
sw.cast(373, 'pf_hpconversion', 'self', after_ms=1800)
sw.cast(374, 'pf_soulchange', 'enemy', cast_ms=3000, after_ms=5000)
sw.cast(375, 'pf_soulburn', 'enemy')
sw.cast(402, 'pf_mindbreaker', 'enemy', after_ms=1200)
sw.cast(403, 'pf_memorize', 'self', cast_ms=5000)
sw.cast(482, 'pf_doublecasting', 'self', cast_ms=2000)
sw.cast(404, 'pf_fogwall', 'ground')
sw.cast(405, 'pf_spiderweb', 'enemy')
sw.leave_and_return('units')
STEPS = sw.end()
