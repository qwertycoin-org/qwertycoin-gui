import hashlib
import importlib.util
from pathlib import Path
import tempfile
import unittest


MODULE_PATH = Path(__file__).resolve().parents[1] / "validate_candidate_files.py"
SPEC = importlib.util.spec_from_file_location("validate_candidate_files", MODULE_PATH)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class ValidateCandidateFilesTests(unittest.TestCase):
    artifact = "qwertycoin-gui-v2.0.2-macos-arm64"

    def create_complete_set(self, directory):
        names = [
            f"{self.artifact}.tar.gz",
            f"{self.artifact}.sha256",
            f"{self.artifact}.dmg",
            f"{self.artifact}.dmg.sha256",
        ]
        for name in names[:-1]:
            (directory / name).write_bytes((name + "\n").encode())
        digest = hashlib.sha256((directory / names[2]).read_bytes()).hexdigest()
        (directory / names[3]).write_text(f"{digest}  {names[2]}\n", encoding="ascii")
        return names

    def test_complete_set_and_exact_dmg_digest(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            names = self.create_complete_set(directory)
            MODULE.validate(directory, names, names[3], names[2])

    def test_missing_dmg_is_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            names = self.create_complete_set(directory)
            (directory / names[2]).unlink()
            with self.assertRaisesRegex(ValueError, "missing="):
                MODULE.validate(directory, names, names[3], names[2])

    def test_wrong_dmg_digest_is_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            names = self.create_complete_set(directory)
            (directory / names[3]).write_text(f"{'0' * 64}  {names[2]}\n", encoding="ascii")
            with self.assertRaisesRegex(ValueError, "digest mismatch"):
                MODULE.validate(directory, names, names[3], names[2])

    def test_unexpected_file_is_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            names = self.create_complete_set(directory)
            (directory / "unexpected.txt").write_text("unexpected", encoding="ascii")
            with self.assertRaisesRegex(ValueError, "unexpected="):
                MODULE.validate(directory, names, names[3], names[2])

    def test_checksum_cannot_name_another_path(self):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            names = self.create_complete_set(directory)
            digest = hashlib.sha256((directory / names[2]).read_bytes()).hexdigest()
            (directory / names[3]).write_text(f"{digest}  ../{names[2]}\n", encoding="ascii")
            with self.assertRaisesRegex(ValueError, "invalid format"):
                MODULE.validate(directory, names, names[3], names[2])


if __name__ == "__main__":
    unittest.main()
