"""The skill sweep for the Merchant line (parity66): Merchant, Blacksmith, Alchemist, Whitesmith and
Creator with a cart and an axe; the shop and production windows the skills open closed again, a
homunculus called and rested, the plants and bottles thrown, Demonstration left and come back to.

Cast here, by class:
Merchant (5): MC_IDENTIFY, MC_VENDING, MC_MAMMONITE, MC_CARTREVOLUTION, MC_CHANGECART, MC_LOUD,
    ALL_BUYING_STORE
Blacksmith (10): BS_REPAIRWEAPON, BS_HAMMERFALL, BS_ADRENALINE, BS_WEAPONPERFECT, BS_OVERTHRUST, BS_MAXIMIZE,
    BS_GREED, BS_ADRENALINE2
Alchemist (18): AM_CALLHOMUN, AM_REST, AM_RESURRECTHOMUN, AM_PHARMACY, AM_ACIDTERROR, AM_POTIONPITCHER,
    AM_CANNIBALIZE, AM_SPHEREMINE, AM_CP_WEAPON, AM_CP_SHIELD, AM_CP_ARMOR, AM_CP_HELM, AM_BERSERKPITCHER,
    AM_TWILIGHT1, AM_TWILIGHT2, AM_TWILIGHT3, AM_DEMONSTRATION
Whitesmith (4011): WS_CARTBOOST, WS_MELTDOWN, WS_WEAPONREFINE, WS_CARTTERMINATION, WS_OVERTHRUSTMAX
Creator (4019): CR_SLIMPITCHER, CR_FULLPROTECTION, CR_ACIDDEMONSTRATION, CR_CULTIVATION
"""
from skill_sweep import Sweep


# The shortcut bar, slot by slot (seedSkillSweep in server/paritysrv_test.go seeds the same).
HOTBAR = [40, 41, 42, 153, 154, 155, 2535, 108, 110, 111, 112, 113, 114, 1013, 459, 243, 244, 247, 228, 230,
          231, 232, 233, 234, 235, 236, 237, 446, 496, 497, 498, 229, 387, 384, 477, 485, 486, 478, 479, 490,
          491]

sw = Sweep('parity66', HOTBAR)

# Merchant
sw.job(5, 'merchant', setup=['@cart 1', '@parityequip 1301', '@item 6377 3'])
sw.s.baseline()
sw.cast(40, 'mc_identify', 'self')
sw.s.close_new()
sw.s.baseline()
sw.cast(41, 'mc_vending', 'self')
sw.s.close_new()
sw.cast(42, 'mc_mammonite', 'enemy')
sw.cast(153, 'mc_cartrevolution', 'enemy')
sw.s.baseline()
sw.cast(154, 'mc_changecart', 'self')
sw.s.close_new()
sw.cast(155, 'mc_loud', 'self')
sw.s.baseline()
sw.cast(2535, 'all_buying_store', 'self')
sw.s.close_new()

# Blacksmith
sw.job(10, 'blacksmith', setup=['@parityequip 1301'])
sw.s.baseline()
sw.cast(108, 'bs_repairweapon', 'friend', cast_ms=7500)
sw.s.close_new()
sw.cast(110, 'bs_hammerfall', 'ground', near=True)
sw.cast(111, 'bs_adrenaline', 'self')
sw.cast(112, 'bs_weaponperfect', 'self')
sw.cast(113, 'bs_overthrust', 'self')
sw.cast(114, 'bs_maximize', 'self')
sw.cast(114, 'bs_maximize_off', 'self')
sw.cast(1013, 'bs_greed', 'self', after_ms=1000)
sw.cast(459, 'bs_adrenaline2', 'self')

# Alchemist
sw.job(18, 'alchemist', setup=['@parityequip 1301', '@item 7142 3', '@item 7134 3', '@item 7136 3', '@item 501 3', '@item 502 3', '@item 503 3', '@item 504 3', '@item 505 3', '@item 7137 3', '@item 7138 3', '@item 7139 3', '@item 657 6', '@item 7135 3'])
sw.cast(243, 'am_callhomun', 'self')
sw.cast(244, 'am_rest', 'self')
sw.cast(247, 'am_resurrecthomun', 'self', cast_ms=2000)
sw.cast(228, 'am_pharmacy', 'self')
sw.s.click_ui('UIMessageBox/UIBitmapButton#176', then=20)
sw.cast(230, 'am_acidterror', 'enemy', cast_ms=1000)
sw.cast(231, 'am_potionpitcher', 'friend', after_ms=500)
sw.cast(232, 'am_cannibalize', 'ground', cast_ms=2000, after_ms=500)
sw.cast(233, 'am_spheremine', 'ground', cast_ms=2000, after_ms=500, near=True)
sw.cast(234, 'am_cp_weapon', 'friend', cast_ms=2000)
sw.cast(235, 'am_cp_shield', 'friend', cast_ms=2000)
sw.cast(236, 'am_cp_armor', 'friend', cast_ms=2000)
sw.cast(237, 'am_cp_helm', 'friend', cast_ms=2000)
sw.cast(446, 'am_berserkpitcher', 'friend', after_ms=500)
sw.cast(496, 'am_twilight1', 'self', cast_ms=3000, after_ms=10000)
sw.cast(497, 'am_twilight2', 'self', cast_ms=3000, after_ms=10000)
sw.cast(498, 'am_twilight3', 'self', cast_ms=3000, after_ms=10000)
sw.cast(229, 'am_demonstration', 'ground', cast_ms=1000)
sw.leave_and_return('units')

# Whitesmith
sw.job(4011, 'whitesmith', setup=['@cart 1', '@parityequip 1301'])
sw.cast(387, 'ws_cartboost', 'self')
sw.cast(384, 'ws_meltdown', 'self', cast_ms=1000)
sw.cast(477, 'ws_weaponrefine', 'self')
sw.s.click_ui('UIWeaponRefineWnd/UIBitmapButton#177', then=20)
sw.cast(485, 'ws_carttermination', 'enemy')
sw.cast(486, 'ws_overthrustmax', 'self')

# Creator
sw.job(4019, 'creator', setup=['@parityequip 1301', '@item 545 3', '@item 546 3', '@item 547 3', '@item 7139 3', '@item 7135 3', '@item 7136 3', '@item 921 3', '@item 905 3'])
sw.cast(478, 'cr_slimpitcher', 'ground', cast_ms=1000, after_ms=1000, near=True)
sw.cast(479, 'cr_fullprotection', 'friend', cast_ms=2000)
sw.cast(490, 'cr_aciddemonstration', 'enemy', cast_ms=1000, after_ms=1000)
sw.cast(491, 'cr_cultivation', 'ground', near=True)
STEPS = sw.end()
