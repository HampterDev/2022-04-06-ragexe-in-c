"""charselect's session on a msgStringTable.txt whose every entry is empty: every caption, label and box sized to
its message gets no width, and a window with no canvas must draw nothing rather than fail."""
import os
import runpy

VARIANT_OF = 'charselect'
VARIANT = 'msg_empty:0-5000'

# the base scenario's session, played against its recording with the data varied
_base = runpy.run_path(os.path.join(os.path.dirname(os.path.abspath(__file__)), VARIANT_OF + '.py'))
globals().update({k: _base[k] for k in ('STEPS', 'PHASES', 'BOT', 'ANSWER_QUIT') if k in _base})
