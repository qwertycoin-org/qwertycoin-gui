#!/usr/bin/env python3

from importlib.util import module_from_spec, spec_from_file_location
from pathlib import Path
import tempfile
import unittest


MODULE_PATH = Path(__file__).resolve().parents[1] / "verify_candidate_archive.py"
SPEC = spec_from_file_location("verify_candidate_archive", MODULE_PATH)
assert SPEC and SPEC.loader
VERIFY = module_from_spec(SPEC)
SPEC.loader.exec_module(VERIFY)


class ArchivePathTests(unittest.TestCase):
    def assert_rejected(self, callback) -> None:
        with self.assertRaises(SystemExit):
            callback()

    def test_rejects_absolute_and_traversal_paths(self) -> None:
        self.assert_rejected(lambda: VERIFY.normalize_member("/payload"))
        self.assert_rejected(lambda: VERIFY.normalize_member("payload/../escape"))
        self.assert_rejected(lambda: VERIFY.normalize_member("payload\\escape"))

    def test_rejects_casefold_collision(self) -> None:
        self.assert_rejected(
            lambda: VERIFY.validate_names(["release/File", "release/file"], "release")
        )

    def test_rejects_members_outside_expected_root(self) -> None:
        self.assert_rejected(lambda: VERIFY.validate_names(["other/file"], "release"))

    def test_accepts_only_internal_symlink_targets(self) -> None:
        VERIFY.validate_link("release/lib/libname.so", "libname.so.1", "release")
        self.assert_rejected(
            lambda: VERIFY.validate_link("release/lib/libname.so", "../../../escape", "release")
        )

    def test_accepts_text_and_binary_sha256_manifest_formats(self) -> None:
        digest = "a" * 64
        self.assertIsNotNone(VERIFY.SHA256_LINE.fullmatch(f"{digest}  release/file"))
        self.assertIsNotNone(VERIFY.SHA256_LINE.fullmatch(f"{digest} *release/file"))

    def test_release_readme_must_match_packaged_core(self) -> None:
        expected_core = "1" * 40
        with tempfile.TemporaryDirectory() as directory:
            readme = Path(directory) / "README.md"
            readme.write_text(f"Core revision: {expected_core}\n", encoding="utf-8")
            VERIFY.verify_release_readme(readme, expected_core)

            readme.write_text(f"Core revision: {'2' * 40}\n", encoding="utf-8")
            self.assert_rejected(
                lambda: VERIFY.verify_release_readme(readme, expected_core)
            )

    def test_windows_payload_requires_all_runtime_qml_modules(self) -> None:
        required = (
            "qwertycoin-gui.exe",
            "qwertycoind.exe",
            "qwertycoin-wallet-cli.exe",
            "qwertycoin-wallet-rpc.exe",
            "platforms/qwindows.dll",
            "imageformats/qsvg.dll",
            "QtQuick/Controls/qtquickcontrolsplugin.dll",
            "QtQuick/Controls.2/qtquickcontrols2plugin.dll",
            "Qt/labs/platform/qtlabsplatformplugin.dll",
        )
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for relative in required:
                candidate = root / relative
                candidate.parent.mkdir(parents=True, exist_ok=True)
                candidate.write_bytes(b"fixture")

            self.assert_rejected(lambda: VERIFY.require_payload(root, "windows"))

            graphical_effects = root / "QtGraphicalEffects/qmldir"
            graphical_effects.parent.mkdir(parents=True)
            graphical_effects.write_text(
                "module QtGraphicalEffects\n", encoding="utf-8"
            )

            self.assert_rejected(lambda: VERIFY.require_payload(root, "windows"))

            for relative in (
                "Qt5Multimedia.dll",
                "QtMultimedia/qmldir",
                "QtMultimedia/declarative_multimedia.dll",
            ):
                candidate = root / relative
                candidate.parent.mkdir(parents=True, exist_ok=True)
                candidate.write_bytes(b"fixture")
            VERIFY.require_payload(root, "windows")


if __name__ == "__main__":
    unittest.main()
