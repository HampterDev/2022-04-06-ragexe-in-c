"""Typing: the command palette over the chat line ('/' and '@' commands filtered as they are
typed, a row picked with the arrows), and word editing in the chat line with the word keys on
(the default) and off (/wordkeys)."""
from common import scenario



def clear(s, n):
    for _ in range(n):
        s.key('back', then=1)
    return s.wait(10)


s = scenario('parity14')
s.type('/', then=20).snap('palette_slash')
s.type('wh', then=20).snap('palette_wh')
s.key('down', then=10).snap('palette_down')
s.key('up', then=10).snap('palette_up')
clear(s, 3).snap('palette_cleared')
s.type('@ite', then=20).snap('palette_at_ite')
clear(s, 4)
s.type('one two three', then=10).key('ctrl+back', then=10).snap('word_back')
s.key('ctrl+left', then=10).type('X', then=10).snap('word_left')
s.key('ctrl+right', then=10).type('Y', then=10).snap('word_right')
s.key('home', then=5).key('ctrl+delete', then=10).snap('word_delete')
s.key('enter', then=30).snap('word_sent')
s.say('/wordkeys', then=30).snap('wordkeys_off')
s.type('one two', then=10).key('ctrl+back', then=10).snap('word_back_off')
s.key('ctrl+left', then=10).type('X', then=10).snap('word_left_off')
s.key('enter', then=30)
s.say('/wordkeys', then=30).snap('wordkeys_on')
s.quit()
STEPS = s.steps
