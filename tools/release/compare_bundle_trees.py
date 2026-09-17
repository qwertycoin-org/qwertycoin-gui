#!/usr/bin/env python3
import argparse
import hashlib
import os
from pathlib import Path
import stat
import sys


def digest(path):
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(chunk)
    return value.hexdigest()


def inventory(root):
    result = {}

    def visit(directory, relative):
        with os.scandir(directory) as entries:
            for entry in sorted(entries, key=lambda item: item.name):
                entry_path = Path(entry.path)
                entry_relative = relative / entry.name
                metadata = entry.stat(follow_symlinks=False)
                mode = stat.S_IMODE(metadata.st_mode)
                key = entry_relative.as_posix()
                if stat.S_ISLNK(metadata.st_mode):
                    result[key] = ("symlink", mode, os.readlink(entry_path))
                elif stat.S_ISDIR(metadata.st_mode):
                    result[key] = ("directory", mode)
                    visit(entry_path, entry_relative)
                elif stat.S_ISREG(metadata.st_mode):
                    result[key] = ("file", mode, metadata.st_size, digest(entry_path))
                else:
                    raise ValueError(f"unsupported filesystem object: {key}")

    visit(root, Path())
    return result


def compare(left, right):
    left_inventory = inventory(left)
    right_inventory = inventory(right)
    if left_inventory == right_inventory:
        return len(left_inventory)

    left_keys = set(left_inventory)
    right_keys = set(right_inventory)
    messages = []
    for key in sorted(left_keys - right_keys):
        messages.append(f"missing from copy: {key}")
    for key in sorted(right_keys - left_keys):
        messages.append(f"unexpected in copy: {key}")
    for key in sorted(left_keys & right_keys):
        if left_inventory[key] != right_inventory[key]:
            messages.append(
                f"metadata/content mismatch: {key}: "
                f"{left_inventory[key]!r} != {right_inventory[key]!r}"
            )
    raise ValueError("\n".join(messages[:50]))


def main():
    parser = argparse.ArgumentParser(
        description="Compare app-bundle content, symlinks and permission modes exactly."
    )
    parser.add_argument("source", type=Path)
    parser.add_argument("copy", type=Path)
    args = parser.parse_args()
    for path in (args.source, args.copy):
        if not path.is_dir() or path.is_symlink():
            parser.error(f"not a safe application bundle directory: {path}")
    try:
        count = compare(args.source, args.copy)
    except ValueError as error:
        print(error, file=sys.stderr)
        return 1
    print(f"Bundle trees match exactly: {count} entries")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
