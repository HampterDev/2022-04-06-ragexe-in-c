"""Every chat command of herc-client's command table, typed into the chat input. An on/off
command is given twice (its state back where it was); a command that opens a window is
given bare and with a search, and whatever it opened is closed again. Then the original
client's commands herc-client changed (/h, /who)."""
from common import scenario

TOGGLES = ['/talktest', '/nodmg', '/spritefilter', '/spritedepth', '/dmgfilter', '/mvpaura', '/mapculling',
           '/mapquality', '/stats', '/targetlock', '/quickcast', '/quickcastaim', '/quickcastcursor', '/quickcastsnap',
           '/castarea', '/targetoutline', '/targetindicator', '/ignoreblocked', '/refuseblocked', '/smoothself',
           '/smoothplayers', '/smoothmonsters', '/altpathing', '/companionlooks', '/mountlooks', '/spbar',
           '/partybuffs', '/itembind', '/equiponly', '/hotbarfade', '/chatfade', '/wordkeys', '/cmdpalette',
           '/boldnames', '/hold']
ACTIONS = ['/cachestat', '/ping', '/ping reset', '/showpath', '/uitext', '/h', '/who']
WINDOWS = ['/mobinfo', '/mobinfo poring', '/mi', '/whereis poring', '/whodrops', '/whodrops jellopy', '/wd',
           '/iteminfo jellopy', '/meter', '/killcount', '/kc', '/recipes', '/guildlog', '/glog', '/whosell',
           '/whosell jellopy', '/ws', '/whobuy', '/wb', '/beams']


s = scenario('parity02')
s.snap('map')
for i, cmd in enumerate(TOGGLES):
    s.say(cmd, then=20).snap(f'toggle_{cmd[1:]}_1')
    s.say(cmd, then=20).snap(f'toggle_{cmd[1:]}_2')
for cmd in ACTIONS:
    s.baseline().say(cmd, then=40).snap('action_' + cmd[1:].replace(' ', '_')).close_new()
for cmd in WINDOWS:
    s.baseline().say(cmd, then=60).snap('window_' + cmd[1:].replace(' ', '_')).close_new()
s.snap('done')
s.quit()
STEPS = s.steps
