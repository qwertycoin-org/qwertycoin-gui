#!/usr/bin/env python3
import argparse
import hashlib
import hmac
from pathlib import Path
import re
import sys


SAFE_NAME = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]{0,255}$")
CHECKSUM_LINE = re.compile(r"^([0-9a-f]{64})  ([A-Za-z0-9][A-Za-z0-9._-]{0,255})$")


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def validate(directory, expected_names, checksum_file=None, checksum_target=None):
    expected = set(expected_names)
    if len(expected) != len(expected_names) or not expected:
        raise ValueError("expected filenames must be unique and non-empty")
    for name in expected:
        if not SAFE_NAME.fullmatch(name):
            raise ValueError(f"unsafe expected filename: {name}")

    actual = set()
    for entry in directory.iterdir():
        if entry.is_symlink() or not entry.is_file():
            raise ValueError(f"candidate input is not a regular file: {entry.name}")
        actual.add(entry.name)
    if actual != expected:
        missing = sorted(expected - actual)
        unexpected = sorted(actual - expected)
        raise ValueError(f"candidate file set mismatch; missing={missing}, unexpected={unexpected}")

    if (checksum_file is None) != (checksum_target is None):
        raise ValueError("checksum file and target must be provided together")
    if checksum_file is None:
        return
    if checksum_file not in expected or checksum_target not in expected:
        raise ValueError("checksum file and target must be members of the expected set")

    try:
        content = (directory / checksum_file).read_text(encoding="ascii")
    except UnicodeDecodeError as error:
        raise ValueError("checksum file is not ASCII") from error
    lines = content.splitlines()
    if len(lines) != 1:
        raise ValueError("checksum file must contain exactly one line")
    match = CHECKSUM_LINE.fullmatch(lines[0])
    if not match:
        raise ValueError("checksum file has an invalid format")
    expected_digest, declared_name = match.groups()
    if declared_name != checksum_target:
        raise ValueError(f"checksum target mismatch: {declared_name}")
    actual_digest = sha256(directory / checksum_target)
    if not hmac.compare_digest(expected_digest, actual_digest):
        raise ValueError("checksum digest mismatch")


def main():
    parser = argparse.ArgumentParser(description="Validate an exact native candidate file set.")
    parser.add_argument("--directory", required=True, type=Path)
    parser.add_argument("--expected", required=True, action="append")
    parser.add_argument("--checksum-file")
    parser.add_argument("--checksum-target")
    args = parser.parse_args()
    if not args.directory.is_dir() or args.directory.is_symlink():
        parser.error(f"not a safe candidate directory: {args.directory}")
    try:
        validate(
            args.directory,
            args.expected,
            checksum_file=args.checksum_file,
            checksum_target=args.checksum_target,
        )
    except ValueError as error:
        print(error, file=sys.stderr)
        return 1
    print(f"Validated exact candidate file set: {len(args.expected)} files")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
