from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[3]


class UpdaterContractTests(unittest.TestCase):
    def read(self, relative_path):
        return (ROOT / relative_path).read_text(encoding="utf-8")

    def test_release_builds_enable_exact_platform_tags(self):
        workflow = self.read(".github/workflows/release.yml")
        self.assertEqual(workflow.count("-DWITH_UPDATER=ON"), 3)
        for build_tag in ("linux-x64", "mac-armv8", "win-x64"):
            self.assertIn(f"-DBUILD_TAG={build_tag}", workflow)

    def test_gui_uses_qwc_dnssec_metadata_without_inherited_signers(self):
        manager = self.read("src/libwalletqt/WalletManager.cpp")
        main_qml = self.read("main.qml")
        dialog = self.read("components/UpdateDialog.qml")

        self.assertIn("UpdateMetadata().check", manager)
        self.assertIn("#ifndef WITH_UPDATER", manager)
        self.assertIn("DNSSEC-validated update found", manager)
        self.assertNotIn("fetchSignedHash", manager)
        self.assertIn('"qwertycoin-gui", "gui", getBuildTag(), Version.GUI_VERSION_NUMBER', main_qml)
        self.assertIn("property bool qwcCheckForUpdates: true", main_qml)
        self.assertNotIn("persistentSettings.checkForUpdates", main_qml)
        self.assertIn("Version.GUI_VERSION_NUMBER", main_qml)
        self.assertNotIn("getBuildTag(), version[0]", main_qml)
        self.assertIn("SHA-256 verified against DNSSEC metadata", dialog)
        self.assertNotIn("signature verified", dialog)

    def test_legacy_monero_update_trust_is_not_shipped(self):
        self.assertNotIn("verify-update", self.read("src/main/main.cpp"))
        self.assertNotIn("add_subdirectory(openpgp)", self.read("src/CMakeLists.txt"))
        resources = self.read("qml.qrc")
        for inherited_key in ("binaryfate.asc", "fluffypony.asc", "luigi1111.asc"):
            self.assertNotIn(inherited_key, resources)


if __name__ == "__main__":
    unittest.main()
