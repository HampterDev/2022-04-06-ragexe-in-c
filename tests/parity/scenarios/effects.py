"""Effects on the character: a Priest's Assumptio (the glow under the sprite and its pulse), and
the session ended by the server (the Disconnected box)."""
from common import scenario

ME = 'Parity27'

s = scenario('parity27')
s.say('@baselvl 99', then=30).say('@joblvl 50', then=30).say('@job 8', then=60).say('@allskill', then=40)
s.say(f'@useskill 361 5 {ME}', then=40).snap('assumptio')
for n in range(4):
    s.wait(8).snap(f'assumptio_pulse_{n}')
s.say(f'@kick {ME}', then=60).snap('disconnected')
s.wait(200).snap('after_disconnect')
s.quit()
STEPS = s.steps
