"""vending's session with every item named "^FF00^^ab^0000FFname^" (unbalanced colour codes, and wider than the
item count window's label): a name is cut to its label with "..." wherever the client cuts it."""
import os
import runpy

VARIANT_OF = 'vending'
VARIANT = r'lua:System\itemInfo_EN.lua@retype(tbl,identifiedDisplayName/unidentifiedDisplayName,caret)'

# the base scenario's session, played against its recording with the data varied
_base = runpy.run_path(os.path.join(os.path.dirname(os.path.abspath(__file__)), VARIANT_OF + '.py'))
globals().update({k: _base[k] for k in ('STEPS', 'PHASES', 'BOT', 'ANSWER_QUIT') if k in _base})
