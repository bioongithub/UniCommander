"""
Shared test utilities for UniCommander tests.
"""
import os


def entries_for(path):
    """
    Build the expected entries string for a directory, matching
    DirectoryPanel::refresh() ordering:
      .. (if not root), dirs case-insensitive, files case-insensitive.
    Directories have a trailing '/'.
    """
    names = os.listdir(path)
    dirs  = sorted((n for n in names if os.path.isdir(os.path.join(path, n))),
                   key=str.casefold)
    files = sorted((n for n in names if not os.path.isdir(os.path.join(path, n))),
                   key=str.casefold)

    parts = []
    if not os.path.samefile(path, os.path.dirname(path)):  # not root
        parts.append("../")
    parts += [n + "/" for n in dirs]
    parts += files
    return ",".join(parts)
