"""Find the copy of a data file that the purist client actually reads.

Mirrors the client's lookup (src/fileio/file.c, file_mgr.c, contents/client_lua.c):
  * via the file manager, names get a 'data\\' prefix; data.grf is read first
    unless clientinfo.xml has <readfolder/>, then the loose file;
  * Lua scripts try <name>.lub before <name>.lua; a name "exists" if it is in the
    GRF or on disk;
  * disk-only loads (System\\..., luaL_loadfile) read the loose file only.
"""
import os
import re
import subprocess

from . import grf as grf_mod
from . import rules

DATA = 'data\\'
LUA_PREFIX = 'LuaFiles514\\'


class Source:
    """One concrete file: where it lives and its bytes."""

    def __init__(self, container, path, data):
        self.container = container    # 'data.grf' or 'loose' or 'git:<rev>'
        self.path = path              # path inside the container (backslashes)
        self.data = data

    def __repr__(self):
        return 'Source(%s, %s)' % (self.container, self.path)


class ClientFiles:
    def __init__(self, root):
        self.root = root
        self.grf = grf_mod.Grf(os.path.join(root, 'data.grf'))
        self.read_folder_first = self._read_folder_first()
        self.git_fallbacks = []

    def _read_folder_first(self):
        for name in ('data\\clientinfo.xml', 'clientinfo.xml'):
            p = os.path.join(self.root, name)
            if os.path.isfile(p):
                return re.search(rb'<\s*readfolder\b', open(p, 'rb').read(), re.I) is not None
        return False

    def loose_path(self, rel):
        """Case-insensitive lookup of a loose file; returns the on-disk relative path."""
        parts = rel.replace('/', '\\').split('\\')
        cur = self.root
        found = []
        for part in parts:
            try:
                names = os.listdir(cur)
            except OSError:
                return None
            match = next((n for n in names if n.lower() == part.lower()), None)
            if match is None:
                return None
            found.append(match)
            cur = os.path.join(cur, match)
        return '\\'.join(found) if os.path.isfile(cur) else None

    def loose(self, rel):
        p = self.loose_path(rel)
        if p is None:
            return None
        with open(os.path.join(self.root, p), 'rb') as f:
            return Source('loose', p, f.read())

    def git_head(self, rel):
        """Fallback for a file that is missing on disk: its HEAD version."""
        proc = subprocess.run(['git', '-C', self.root, 'show', 'HEAD:' + rel.replace('\\', '/')],
                              capture_output=True)
        if proc.returncode != 0:
            return None
        self.git_fallbacks.append(rel)
        return Source('git:HEAD', rel, proc.stdout)

    def in_grf(self, rel):
        name = self.grf.find(rel.encode('cp949'))
        return Source('data.grf', name.decode('cp949'), self.grf.read(name)) if name else None

    def via_file_manager(self, rel):
        """file_open(name, 0, 1, 0): 'data\\' + name, GRF first unless <readfolder/>."""
        first, second = (self.loose, self.in_grf) if self.read_folder_first else (self.in_grf, self.loose)
        return first(DATA + rel) or second(DATA + rel)

    def exists_via_file_manager(self, rel):
        return self.grf.find((DATA + rel).encode('cp949')) is not None or \
            self.loose_path(DATA + rel) is not None

    def resolve(self, spec):
        """The Source the client loads for a rules.Spec, or None."""
        if spec.loader == rules.CFILE_LUA:
            for ext in ('.lub', '.lua'):
                rel = LUA_PREFIX + spec.name + ext
                if self.exists_via_file_manager(rel):
                    return self.via_file_manager(rel)
            return None
        if spec.loader == rules.CFILE_TABLE:
            return self.via_file_manager(spec.name)
        if spec.loader == rules.DISK_LUA:
            for ext in ('.lub', '.lua'):
                src = self.loose(spec.name + ext)
                if src:
                    return src
            return self.git_head(spec.name + '.lub')
        if spec.loader == rules.DISK_MAIN:
            return self.loose(spec.name) or self.git_head(spec.name)
        raise ValueError(spec.loader)

    def shadowed(self, spec, active):
        """Loose copies that exist but lose against `active` (GRF wins, or .lub beats .lua)."""
        out = []
        if spec.loader == rules.CFILE_LUA:
            for ext in ('.lub', '.lua'):
                src = self.loose(DATA + LUA_PREFIX + spec.name + ext)
                if src and not (active.container == 'loose' and active.path.lower() == src.path.lower()):
                    out.append(src)
        elif spec.loader == rules.CFILE_TABLE:
            src = self.loose(DATA + spec.name)
            if src and not (active.container == 'loose' and active.path.lower() == src.path.lower()):
                out.append(src)
        return out
