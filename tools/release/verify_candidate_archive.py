#!/usr/bin/env python3
"""Safely extract and verify one native Qwertycoin GUI candidate archive."""

from __future__ import annotations

import argparse
import hashlib
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import stat
import tarfile
import zipfile


SHA256_LINE = re.compile(r"^([0-9a-f]{64}) ([ *])(.+)$")


def fail(message: str) -> None:
    raise SystemExit(message)


def normalize_member(name: str) -> str:
    if not name or "\0" in name or "\\" in name:
        fail(f"unsafe archive member name: {name!r}")
    stripped = name.rstrip("/")
    path = PurePosixPath(stripped)
    if not stripped or path.is_absolute() or any(part in ("", ".", "..") for part in path.parts):
        fail(f"unsafe archive member path: {name!r}")
    return str(path)


def require_expected_member(name: str, root_name: str) -> None:
    if name == f"{root_name}.sha256":
        return
    if name != root_name and not name.startswith(f"{root_name}/"):
        fail(f"archive member is outside the expected root: {name}")


def validate_link(name: str, target: str, root_name: str) -> None:
    if not target or "\0" in target or "\\" in target:
        fail(f"unsafe symlink target for {name}: {target!r}")
    target_path = PurePosixPath(target)
    if target_path.is_absolute():
        fail(f"absolute symlink target for {name}: {target}")

    combined = PurePosixPath(name).parent.joinpath(target_path)
    parts: list[str] = []
    for part in combined.parts:
        if part in ("", "."):
            continue
        if part == "..":
            if not parts:
                fail(f"symlink escapes archive root: {name} -> {target}")
            parts.pop()
        else:
            parts.append(part)
    resolved = "/".join(parts)
    if resolved != root_name and not resolved.startswith(f"{root_name}/"):
        fail(f"symlink escapes expected root: {name} -> {target}")


def validate_names(names: list[str], root_name: str) -> None:
    seen: set[str] = set()
    casefolded: dict[str, str] = {}
    for raw_name in names:
        name = normalize_member(raw_name)
        require_expected_member(name, root_name)
        if name in seen:
            fail(f"duplicate archive member: {name}")
        seen.add(name)
        folded = name.casefold()
        if folded in casefolded and casefolded[folded] != name:
            fail(f"case-folding archive collision: {casefolded[folded]} and {name}")
        casefolded[folded] = name


def extract_tar(archive: Path, destination: Path, root_name: str) -> None:
    with tarfile.open(archive, "r:gz") as package:
        members = package.getmembers()
        validate_names([member.name for member in members], root_name)
        for member in members:
            name = normalize_member(member.name)
            if member.islnk():
                fail(f"hard links are not permitted in release archives: {name}")
            if member.issym():
                validate_link(name, member.linkname, root_name)
            elif not (member.isfile() or member.isdir()):
                fail(f"unsupported special archive member: {name}")
        # Member paths, link targets and types were fully validated above.
        # Avoid tarfile's newer filter= parameter so the verifier also works
        # with the Python version shipped by Ubuntu 22.04 review hosts.
        package.extractall(destination)


def extract_zip(archive: Path, destination: Path, root_name: str) -> None:
    with zipfile.ZipFile(archive) as package:
        members = package.infolist()
        validate_names([member.filename for member in members], root_name)
        for member in members:
            mode = member.external_attr >> 16
            if stat.S_ISLNK(mode):
                fail(f"symlinks are not permitted in Windows release archives: {member.filename}")
        package.extractall(destination)


def regular_files(root: Path) -> set[str]:
    found: set[str] = set()
    for directory, subdirectories, filenames in os.walk(root, followlinks=False):
        subdirectories[:] = [
            name for name in subdirectories if not Path(directory, name).is_symlink()
        ]
        for filename in filenames:
            candidate = Path(directory, filename)
            if stat.S_ISREG(candidate.lstat().st_mode):
                found.add(candidate.relative_to(root.parent).as_posix())
    return found


def verify_manifest(destination: Path, root_name: str) -> int:
    manifest = destination / f"{root_name}.sha256"
    if not manifest.is_file() or manifest.is_symlink():
        fail("candidate archive does not contain a regular SHA-256 manifest")

    expected: dict[str, str] = {}
    for line_number, line in enumerate(manifest.read_text(encoding="utf-8").splitlines(), 1):
        match = SHA256_LINE.fullmatch(line)
        if not match:
            fail(f"invalid SHA-256 manifest line {line_number}")
        digest, _mode, raw_name = match.groups()
        name = normalize_member(raw_name)
        if name == f"{root_name}.sha256":
            fail("SHA-256 manifest must not hash itself")
        require_expected_member(name, root_name)
        if name in expected:
            fail(f"duplicate SHA-256 manifest path: {name}")
        expected[name] = digest

    root = destination / root_name
    if not root.is_dir() or root.is_symlink():
        fail("candidate archive root is missing or is not a directory")
    actual = regular_files(root)
    if actual != set(expected):
        missing = sorted(actual - set(expected))
        extra = sorted(set(expected) - actual)
        fail(f"SHA-256 manifest coverage mismatch; unhashed={missing[:3]}, absent={extra[:3]}")

    for name, expected_digest in expected.items():
        digest = hashlib.sha256((destination / name).read_bytes()).hexdigest()
        if digest != expected_digest:
            fail(f"SHA-256 mismatch: {name}")
    return len(expected)


def parse_build_info(path: Path) -> dict[str, str]:
    if not path.is_file() or path.is_symlink():
        fail("BUILD-INFO.txt is missing or not a regular file")
    values: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if "=" not in line:
            fail(f"invalid BUILD-INFO line: {line!r}")
        key, value = line.split("=", 1)
        if not key or key in values:
            fail(f"invalid or duplicate BUILD-INFO key: {key!r}")
        values[key] = value
    return values


def require_payload(root: Path, platform: str) -> None:
    if platform == "linux":
        required = ["qwertycoin-gui", "qwertycoind", "qwertycoin-wallet-cli", "qwertycoin-wallet-rpc"]
    elif platform == "windows":
        required = [
            "qwertycoin-gui.exe",
            "qwertycoind.exe",
            "qwertycoin-wallet-cli.exe",
            "qwertycoin-wallet-rpc.exe",
            "platforms/qwindows.dll",
            "imageformats/qsvg.dll",
            "QtQuick/Controls/qtquickcontrolsplugin.dll",
            "QtQuick/Controls.2/qtquickcontrols2plugin.dll",
            "Qt/labs/platform/qtlabsplatformplugin.dll",
        ]
    else:
        required = [
            "qwertycoin-gui.app/Contents/Info.plist",
            "qwertycoin-gui.app/Contents/MacOS/qwertycoin-gui",
            "qwertycoin-gui.app/Contents/MacOS/qwertycoind",
            "qwertycoin-gui.app/Contents/MacOS/qwertycoin-wallet-cli",
            "qwertycoin-gui.app/Contents/MacOS/qwertycoin-wallet-rpc",
        ]
    for relative in required:
        candidate = root / relative
        if not candidate.is_file() or candidate.is_symlink():
            fail(f"required {platform} release payload is missing: {relative}")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--archive", type=Path, required=True)
    parser.add_argument("--destination", type=Path, required=True)
    parser.add_argument("--root-name", required=True)
    parser.add_argument("--platform", choices=("linux", "windows", "macos"), required=True)
    parser.add_argument("--expected-source", required=True)
    parser.add_argument("--expected-core", required=True)
    parser.add_argument("--expected-os", required=True)
    parser.add_argument("--expected-arch", required=True)
    parser.add_argument("--expected-qt", required=True)
    parser.add_argument("--expected-glibc-ceiling")
    parser.add_argument("--expected-macos-min-version")
    args = parser.parse_args()

    if args.destination.exists():
        fail(f"refusing to reuse extraction destination: {args.destination}")
    args.destination.mkdir(parents=True)
    try:
        if args.archive.name.endswith(".tar.gz"):
            extract_tar(args.archive, args.destination, args.root_name)
        elif args.archive.suffix == ".zip":
            extract_zip(args.archive, args.destination, args.root_name)
        else:
            fail(f"unsupported release archive type: {args.archive}")

        file_count = verify_manifest(args.destination, args.root_name)
        root = args.destination / args.root_name
        info = parse_build_info(root / "BUILD-INFO.txt")
        expected = {
            "source_revision": args.expected_source,
            "core_revision": args.expected_core,
            "runner_os": args.expected_os,
            "runner_arch": args.expected_arch,
            "qt_version": args.expected_qt,
        }
        if args.expected_glibc_ceiling:
            expected["glibc_ceiling"] = args.expected_glibc_ceiling
        if args.expected_macos_min_version:
            expected["macos_min_version"] = args.expected_macos_min_version
        for key, value in expected.items():
            if info.get(key) != value:
                fail(f"BUILD-INFO mismatch for {key}: expected {value!r}, got {info.get(key)!r}")

        require_payload(root, args.platform)
        print(f"Verified {args.platform} candidate: {file_count} files, exact source/Core metadata")
    except BaseException:
        shutil.rmtree(args.destination, ignore_errors=True)
        raise


if __name__ == "__main__":
    main()
