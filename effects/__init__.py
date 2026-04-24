import ctypes
import os
import glob

_pkg_dir = os.path.dirname(__file__)
_lib_pattern = os.path.join(_pkg_dir, "cext.*")
_lib_files = glob.glob(_lib_pattern)

if not _lib_files:
    raise ImportError("C extension not found, run 'pip install -e .'")

lib = ctypes.CDLL(_lib_files[0])