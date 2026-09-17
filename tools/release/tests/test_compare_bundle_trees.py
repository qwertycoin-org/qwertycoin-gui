import importlib.util
import os
from pathlib import Path
import tempfile
import unittest


MODULE_PATH = Path(__file__).resolve().parents[1] / "compare_bundle_trees.py"
SPEC = importlib.util.spec_from_file_location("compare_bundle_trees", MODULE_PATH)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class CompareBundleTreesTests(unittest.TestCase):
    def create_bundle(self, root):
        executable = root / "Contents" / "MacOS" / "qwertycoin-gui"
        executable.parent.mkdir(parents=True)
        executable.write_bytes(b"wallet-binary")
        executable.chmod(0o755)
        resources = root / "Contents" / "Resources"
        resources.mkdir()
        (resources / "data.txt").write_text("resource\n", encoding="utf-8")
        os.symlink("Versions/Current", root / "Contents" / "Framework")

    def test_identical_trees_match(self):
        with tempfile.TemporaryDirectory() as temp:
            first = Path(temp) / "first.app"
            second = Path(temp) / "second.app"
            self.create_bundle(first)
            self.create_bundle(second)
            self.assertGreater(MODULE.compare(first, second), 0)

    def test_file_content_mismatch_is_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            first = Path(temp) / "first.app"
            second = Path(temp) / "second.app"
            self.create_bundle(first)
            self.create_bundle(second)
            (second / "Contents" / "Resources" / "data.txt").write_text("changed\n")
            with self.assertRaisesRegex(ValueError, "metadata/content mismatch"):
                MODULE.compare(first, second)

    def test_executable_mode_mismatch_is_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            first = Path(temp) / "first.app"
            second = Path(temp) / "second.app"
            self.create_bundle(first)
            self.create_bundle(second)
            (second / "Contents" / "MacOS" / "qwertycoin-gui").chmod(0o644)
            with self.assertRaisesRegex(ValueError, "metadata/content mismatch"):
                MODULE.compare(first, second)

    def test_symlink_target_mismatch_is_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            first = Path(temp) / "first.app"
            second = Path(temp) / "second.app"
            self.create_bundle(first)
            self.create_bundle(second)
            (second / "Contents" / "Framework").unlink()
            os.symlink("Versions/A", second / "Contents" / "Framework")
            with self.assertRaisesRegex(ValueError, "metadata/content mismatch"):
                MODULE.compare(first, second)


if __name__ == "__main__":
    unittest.main()
