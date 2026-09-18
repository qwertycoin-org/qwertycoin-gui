import hashlib
import os
import sys
import tempfile
import unittest
from pathlib import Path


WINDOWS_TOOLS = Path(__file__).resolve().parents[1] / "windows"
sys.path.insert(0, str(WINDOWS_TOOLS))

import package_manifest  # noqa: E402


class WindowsPackageManifestTests(unittest.TestCase):
    def make_package(self, root: Path) -> Path:
        package = root / "qwertycoin-gui-v2.0.1-windows-x86_64"
        (package / "Qt Quick" / "Controls.2").mkdir(parents=True)
        (package / "qwertycoin-gui.exe").write_bytes(b"gui")
        (package / "Qt Quick" / "Controls.2" / "qmldir").write_text(
            "module QtQuick.Controls\n", encoding="utf-8"
        )
        return package

    def test_build_and_verify_exact_package(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            package = self.make_package(root)
            manifest = root / "program-files.sha256"
            include = root / "files.iss"
            entries = package_manifest.collect_entries(package)
            package_manifest.write_manifest(entries, manifest)
            package_manifest.write_inno_file_list(entries, include)

            package_manifest.verify_package(package, manifest)
            contents = include.read_text(encoding="utf-8-sig")
            self.assertIn('DestDir: "{app}"', contents)
            self.assertIn('DestDir: "{app}\\Qt Quick\\Controls.2"', contents)
            self.assertIn("Flags: ignoreversion", contents)
            parsed = package_manifest.read_manifest(manifest)
            gui_entry = next(entry for entry in parsed if entry.path == "qwertycoin-gui.exe")
            self.assertEqual(gui_entry.digest, hashlib.sha256(b"gui").hexdigest())

    def test_changed_missing_and_unexpected_files_fail_verification(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            package = self.make_package(root)
            manifest = root / "program-files.sha256"
            package_manifest.write_manifest(package_manifest.collect_entries(package), manifest)

            (package / "qwertycoin-gui.exe").write_bytes(b"changed")
            with self.assertRaises(package_manifest.ManifestError):
                package_manifest.verify_package(package, manifest)

            (package / "qwertycoin-gui.exe").unlink()
            with self.assertRaises(package_manifest.ManifestError):
                package_manifest.verify_package(package, manifest)

            (package / "unexpected.dll").write_bytes(b"unexpected")
            with self.assertRaises(package_manifest.ManifestError):
                package_manifest.verify_package(package, manifest)

    def test_case_insensitive_collision_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            package = self.make_package(Path(temporary))
            (package / "qt5core.dll").write_bytes(b"lower")
            (package / "Qt5Core.dll").write_bytes(b"upper")
            with self.assertRaisesRegex(package_manifest.ManifestError, "case-insensitive"):
                package_manifest.collect_entries(package)

    def test_links_and_active_user_data_are_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            package = self.make_package(root)
            (package / "qwertycoin-storage").mkdir()
            (package / "qwertycoin-storage" / "settings.ini").write_text("portable=1")
            with self.assertRaisesRegex(package_manifest.ManifestError, "user-data"):
                package_manifest.collect_entries(package)

        if hasattr(os, "symlink"):
            with tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                package = self.make_package(root)
                target = root / "outside.dll"
                target.write_bytes(b"outside")
                try:
                    (package / "linked.dll").symlink_to(target)
                except OSError:
                    self.skipTest("symlinks are not available on this test host")
                with self.assertRaisesRegex(package_manifest.ManifestError, "link/reparse"):
                    package_manifest.collect_entries(package)

    def test_unsafe_windows_paths_are_rejected(self) -> None:
        unsafe = [
            "../escape.dll",
            "/absolute.dll",
            "folder\\backslash.dll",
            "folder//double.dll",
            "CON.txt",
            "folder/trailing./file.dll",
            "folder/name:stream",
            "folder/name;directive.dll",
        ]
        for value in unsafe:
            with self.subTest(value=value):
                with self.assertRaises(package_manifest.ManifestError):
                    package_manifest.validate_relative_path(value)

    def test_manifest_must_be_canonical_and_complete(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            manifest = root / "manifest"
            manifest.write_text(
                package_manifest.MANIFEST_HEADER
                + "\n"
                + "0" * 64
                + "\t1\tB.dll\n"
                + "1" * 64
                + "\t1\ta.dll\n",
                encoding="utf-8",
            )
            with self.assertRaisesRegex(package_manifest.ManifestError, "sorted"):
                package_manifest.read_manifest(manifest)


if __name__ == "__main__":
    unittest.main()
