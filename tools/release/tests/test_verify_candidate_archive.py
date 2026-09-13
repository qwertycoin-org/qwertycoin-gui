#!/usr/bin/env python3

from importlib.util import module_from_spec, spec_from_file_location
from pathlib import Path
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


if __name__ == "__main__":
    unittest.main()
