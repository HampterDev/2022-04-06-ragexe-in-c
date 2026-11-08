"""A Soul Linker (parity30; Estin F1, Estun F2, Esma F3, Kaahi F4, Kaizel F5): the Es bolts and
their trails at a Poring, Kaahi and Kaizel on itself, and a spirit's link with its particles."""
from common import scenario


PORING = 1002
CENTRE = (512, 344)
SC_SOULLINK, SL_ALCHEMIST = 185, 445


def frames(s, name, count, step):
    for n in range(count):
        s.wait(step).snap(f'{name}_{n}')
    return s


s = scenario('parity30')
s.say('@allskill', then=40).say('@monster poring 2', then=90).snap('soul_linker')
for key, name in (('f1', 'estin'), ('f2', 'estun'), ('f3', 'esma')):
    s.key(key, then=10).click_actor(CENTRE, job=PORING, then=0)
    frames(s, name, 4, 6)
    s.wait(40)
s.say('@killmonster', then=40)
s.key('f4', then=60).snap('kaahi')
s.key('f5', then=60).snap('kaizel')
s.say(f'@paritysc {SC_SOULLINK} 60000 5 {SL_ALCHEMIST}', then=10)
frames(s, 'spirit', 4, 10)
s.quit()
STEPS = s.steps
