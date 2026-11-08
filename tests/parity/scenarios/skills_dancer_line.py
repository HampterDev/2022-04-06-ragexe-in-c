"""The skill sweep for the Dancer line (parity64, a female character): Dancer and Gypsy with a whip,
each dance performed in turn and walked out of view and back with, then ended.

Cast here, by class:
Dancer (20): DC_THROWARROW, DC_SCREAM, DC_WINKCHARM, DC_UGLYDANCE, DC_HUMMING, DC_DONTFORGETME,
    DC_FORTUNEKISS, DC_SERVICEFORYOU
Gypsy (4021): none of its own
Not cast here:
BD_ADAPTATION (Dancer): cast as Bard
BD_ENCORE (Dancer): cast as Bard
BD_LULLABY (Dancer): cast as Bard
BD_RICHMANKIM (Dancer): cast as Bard
BD_ETERNALCHAOS (Dancer): cast as Bard
BD_DRUMBATTLEFIELD (Dancer): cast as Bard
BD_RINGNIBELUNGEN (Dancer): cast as Bard
BD_ROKISWEIL (Dancer): cast as Bard
BD_INTOABYSS (Dancer): cast as Bard
BD_SIEGFRIED (Dancer): cast as Bard
CG_ARROWVULCAN (Gypsy): cast as Clown
CG_MOONLIT (Gypsy): cast as Clown
CG_MARIONETTE (Gypsy): cast as Clown
CG_LONGINGFREEDOM (Gypsy): cast as Clown
CG_HERMODE (Gypsy): cast as Clown
CG_TAROTCARD (Gypsy): cast as Clown
"""
from skill_sweep import Sweep


# The shortcut bar, slot by slot (seedSkillSweep in server/paritysrv_test.go seeds the same).
HOTBAR = [324, 326, 1011, 325, 327, 328, 329, 330]

sw = Sweep('parity64', HOTBAR)

# Dancer
sw.job(20, 'dancer', setup=['@parityequip 1950', '@item 1750 300'])
sw.say('@parityequip 1750', then=30)
sw.cast(324, 'dc_throwarrow', 'enemy', cast_ms=1500)
sw.cast(326, 'dc_scream', 'self', after_ms=3000)
sw.cast(1011, 'dc_winkcharm', 'enemy', cast_ms=1000, after_ms=2000)
sw.cast(325, 'dc_uglydance', 'self')
sw.cast(327, 'dc_humming', 'self')
sw.cast(328, 'dc_dontforgetme', 'self')
sw.cast(329, 'dc_fortunekiss', 'self')
sw.cast(330, 'dc_serviceforyou', 'self')
sw.leave_and_return('units')

# Gypsy
sw.job(4021, 'gypsy', setup=['@parityequip 1950'])
STEPS = sw.end()
