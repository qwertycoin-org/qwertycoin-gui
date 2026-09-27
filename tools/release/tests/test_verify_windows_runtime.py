from pathlib import Path
import subprocess
import unittest


ROOT = Path(__file__).resolve().parents[3]
VERIFIER = ROOT / "tools" / "release" / "verify_windows_runtime.sh"


class WindowsRuntimeAllowlistTests(unittest.TestCase):
    def is_system_dll(self, name):
        return subprocess.run(
            [str(VERIFIER), "--is-system-dll", name],
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

    def test_rust_bcrypt_provider_is_system_supplied(self):
        self.assertEqual(self.is_system_dll("bcryptprimitives.dll").returncode, 0)
        self.assertEqual(self.is_system_dll("BCRYPTPRIMITIVES.DLL").returncode, 0)

    def test_unknown_import_is_not_allowlisted(self):
        self.assertNotEqual(self.is_system_dll("qms-evil.dll").returncode, 0)


if __name__ == "__main__":
    unittest.main()
