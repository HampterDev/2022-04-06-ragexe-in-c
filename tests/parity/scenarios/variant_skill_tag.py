"""skills' session with every skill named in a 103-byte run of link tags, longer than the 64-byte buffer the
shout on casting copies it into: the shout runs on into the next buffer as in the original's frame."""
import os
import runpy

VARIANT_OF = 'skills'
VARIANT = r'lua:data\luafiles514\lua files\skillinfoz\skillinfolist.lub@retype(SKILL_INFO_LIST,SkillName/1,tag)'

# the base scenario's session, played against its recording with the data varied
_base = runpy.run_path(os.path.join(os.path.dirname(os.path.abspath(__file__)), VARIANT_OF + '.py'))
globals().update({k: _base[k] for k in ('STEPS', 'PHASES', 'BOT', 'ANSWER_QUIT') if k in _base})
