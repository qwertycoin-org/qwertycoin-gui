#!/usr/bin/env python3
"""Build and verify the exact program-file manifest used by the Windows setup."""

from __future__ import annotations

import argparse
import hashlib
import os
import re
import stat
import sys
from dataclasses import dataclass
from pathlib import Path, PurePosixPath


MANIFEST_HEADER = "# qwertycoin-windows-program-manifest-v1"
HASH_RE = re.compile(r"^[0-9a-f]{64}$")
SAFE_COMPONENT_RE = re.compile(r'^[^<>:"\\|?*;={}\x00-\x1f]+$')
RESERVED_WINDOWS_NAMES = {
    "CON",
    "PRN",
    "AUX",
    "NUL",
    *(f"COM{i}" for i in range(1, 10)),
    *(f"LPT{i}" for i in range(1, 10)),
}
FORBIDDEN_PACKAGE_ROOTS = {
    ".qwertycoin-installer",
    "blockchain",
    "epose-v2",
    "lmdb",
    "qwertycoin-storage",
    "wallets",
}
FORBIDDEN_ACTIVE_FILES = {
    "qwertycoin.conf",
    "settings.ini",
}


class ManifestError(ValueError):
    pass


@dataclass(frozen=True)
class Entry:
    digest: str
    size: int
    path: str


def _is_reparse_point(path: Path) -> bool:
    info = path.lstat()
    attributes = getattr(info, "st_file_attributes", 0)
    reparse_flag = getattr(stat, "FILE_ATTRIBUTE_REPARSE_POINT", 0x400)
    return path.is_symlink() or bool(attributes & reparse_flag)


def _validate_component(component: str) -> None:
    if not component or component in {".", ".."}:
        raise ManifestError(f"unsafe empty or relative component: {component!r}")
    if component.endswith((" ", ".")):
        raise ManifestError(f"Windows path component has a trailing space/dot: {component!r}")
    if not SAFE_COMPONENT_RE.fullmatch(component):
        raise ManifestError(f"Windows path component contains unsupported characters: {component!r}")
    basename = component.split(".", 1)[0].upper()
    if basename in RESERVED_WINDOWS_NAMES:
        raise ManifestError(f"reserved Windows path component: {component!r}")


def validate_relative_path(raw_path: str) -> str:
    if not raw_path or raw_path.startswith(("/", "\\")):
        raise ManifestError(f"path must be relative: {raw_path!r}")
    if "\\" in raw_path or "//" in raw_path:
        raise ManifestError(f"path is not canonical POSIX form: {raw_path!r}")
    pure = PurePosixPath(raw_path)
    if pure.is_absolute() or str(pure) != raw_path:
        raise ManifestError(f"path is not canonical: {raw_path!r}")
    for component in pure.parts:
        _validate_component(component)
    return raw_path


def _reject_user_data_path(relative_path: str) -> None:
    parts = PurePosixPath(relative_path).parts
    if parts[0].casefold() in FORBIDDEN_PACKAGE_ROOTS:
        raise ManifestError(
            f"active user-data root must not be shipped in the program package: {relative_path}"
        )
    if parts[-1].casefold() in FORBIDDEN_ACTIVE_FILES:
        raise ManifestError(
            f"active user configuration must not be shipped in the program package: {relative_path}"
        )


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def collect_entries(package_dir: Path) -> list[Entry]:
    package_dir = package_dir.resolve(strict=True)
    if not package_dir.is_dir():
        raise ManifestError(f"package path is not a directory: {package_dir}")
    if _is_reparse_point(package_dir):
        raise ManifestError(f"package directory must not be a link/reparse point: {package_dir}")

    entries: list[Entry] = []
    casefolded: dict[str, str] = {}
    for root, directories, files in os.walk(package_dir, followlinks=False):
        root_path = Path(root)
        for name in list(directories):
            directory = root_path / name
            relative = directory.relative_to(package_dir).as_posix()
            validate_relative_path(relative)
            if _is_reparse_point(directory):
                raise ManifestError(f"package contains a directory link/reparse point: {relative}")
        for name in files:
            source = root_path / name
            relative = validate_relative_path(source.relative_to(package_dir).as_posix())
            _reject_user_data_path(relative)
            if _is_reparse_point(source):
                raise ManifestError(f"package contains a file link/reparse point: {relative}")
            if not source.is_file():
                raise ManifestError(f"package contains a non-regular file: {relative}")
            folded = relative.casefold()
            if folded in casefolded:
                raise ManifestError(
                    f"case-insensitive path collision: {casefolded[folded]!r} and {relative!r}"
                )
            casefolded[folded] = relative
            entries.append(Entry(_sha256(source), source.stat().st_size, relative))

    entries.sort(key=lambda entry: entry.path.casefold())
    if not entries:
        raise ManifestError("program package is empty")
    return entries


def write_manifest(entries: list[Entry], destination: Path) -> None:
    lines = [MANIFEST_HEADER]
    lines.extend(f"{entry.digest}\t{entry.size}\t{entry.path}" for entry in entries)
    destination.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")


def _iss_escape(value: str) -> str:
    return value.replace('"', '""')


def write_inno_file_list(entries: list[Entry], destination: Path) -> None:
    lines = ["; generated by package_manifest.py; do not edit"]
    for entry in entries:
        path = PurePosixPath(entry.path)
        source = str(path).replace("/", "\\")
        parent = str(path.parent).replace("/", "\\")
        destination_dir = "{app}" if parent == "." else f"{{app}}\\{parent}"
        lines.append(
            'Source: "{#PackageDir}\\%s"; DestDir: "%s"; DestName: "%s"; '
            "Flags: ignoreversion"
            % (
                _iss_escape(source),
                _iss_escape(destination_dir),
                _iss_escape(path.name),
            )
        )
    destination.write_text("\n".join(lines) + "\n", encoding="utf-8-sig", newline="\n")


def read_manifest(manifest: Path) -> list[Entry]:
    try:
        lines = manifest.read_text(encoding="utf-8").splitlines()
    except UnicodeDecodeError as error:
        raise ManifestError("manifest is not valid UTF-8") from error
    if not lines or lines[0] != MANIFEST_HEADER:
        raise ManifestError("manifest header is missing or unsupported")
    entries: list[Entry] = []
    seen: dict[str, str] = {}
    for number, line in enumerate(lines[1:], start=2):
        if not line:
            raise ManifestError(f"blank manifest line {number}")
        fields = line.split("\t")
        if len(fields) != 3:
            raise ManifestError(f"manifest line {number} does not contain three fields")
        digest, size_text, relative = fields
        if not HASH_RE.fullmatch(digest):
            raise ManifestError(f"invalid SHA-256 on manifest line {number}")
        if not size_text.isascii() or not size_text.isdecimal():
            raise ManifestError(f"invalid byte size on manifest line {number}")
        validate_relative_path(relative)
        folded = relative.casefold()
        if folded in seen:
            raise ManifestError(
                f"case-insensitive manifest collision: {seen[folded]!r} and {relative!r}"
            )
        seen[folded] = relative
        entries.append(Entry(digest, int(size_text), relative))
    if not entries:
        raise ManifestError("manifest contains no program files")
    if entries != sorted(entries, key=lambda entry: entry.path.casefold()):
        raise ManifestError("manifest entries are not canonically sorted")
    return entries


def verify_package(package_dir: Path, manifest: Path) -> None:
    expected = read_manifest(manifest)
    actual = collect_entries(package_dir)
    if expected != actual:
        expected_map = {entry.path.casefold(): entry for entry in expected}
        actual_map = {entry.path.casefold(): entry for entry in actual}
        missing = sorted(entry.path for key, entry in expected_map.items() if key not in actual_map)
        unexpected = sorted(entry.path for key, entry in actual_map.items() if key not in expected_map)
        changed = sorted(
            expected_map[key].path
            for key in expected_map.keys() & actual_map.keys()
            if expected_map[key] != actual_map[key]
        )
        raise ManifestError(
            "package does not match manifest "
            f"(missing={missing}, unexpected={unexpected}, changed={changed})"
        )


def _build(arguments: argparse.Namespace) -> None:
    entries = collect_entries(arguments.package_dir)
    write_manifest(entries, arguments.manifest)
    write_inno_file_list(entries, arguments.inno_file_list)
    print(f"manifested {len(entries)} exact program files")


def _verify(arguments: argparse.Namespace) -> None:
    verify_package(arguments.package_dir, arguments.manifest)
    print(f"verified exact program package: {arguments.package_dir}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="command", required=True)

    build = subparsers.add_parser("build")
    build.add_argument("--package-dir", type=Path, required=True)
    build.add_argument("--manifest", type=Path, required=True)
    build.add_argument("--inno-file-list", type=Path, required=True)
    build.set_defaults(handler=_build)

    verify = subparsers.add_parser("verify")
    verify.add_argument("--package-dir", type=Path, required=True)
    verify.add_argument("--manifest", type=Path, required=True)
    verify.set_defaults(handler=_verify)
    return parser.parse_args()


def main() -> int:
    arguments = parse_args()
    try:
        arguments.handler(arguments)
    except (ManifestError, OSError) as error:
        print(f"package manifest error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
