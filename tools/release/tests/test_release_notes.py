"""Exercise the actual assembly notes shell step without publishing anything."""
import os
from pathlib import Path
import subprocess
import tempfile
import textwrap
import unittest


ROOT = Path(__file__).resolve().parents[3]


class ReleaseNotesTests(unittest.TestCase):
    def test_native_installation_names_and_source_disclosures_are_preserved(self):
        workflow = (ROOT / '.github/workflows/assemble-release.yml').read_text()
        step = workflow.split('      - name: Create immutable release notes\n', 1)[1]
        step = step.split('      - name: Detect an exact existing release', 1)[0]
        script = textwrap.dedent(step.split('        run: |\n', 1)[1])
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            (directory / 'release-assets').mkdir()
            notes = directory / 'docs/releases/v2.0.5.md'
            notes.parent.mkdir(parents=True)
            source_notes = (ROOT / 'docs/releases/v2.0.5.md').read_text()
            notes.write_text(source_notes)
            environment = dict(os.environ, RELEASE_KIND='stable', RELEASE_TAG='v2.0.5',
                               EXPECTED_REVISION='a' * 40, CORE_REVISION='b' * 40,
                               GITHUB_REPOSITORY='qwertycoin-org/qwertycoin-gui',
                               LINUX_RUN_ID='1', WINDOWS_RUN_ID='2', MACOS_RUN_ID='3',
                               WINDOWS_ARTIFACT='qwertycoin-gui-v2.0.5-windows-x86_64',
                               MACOS_ARTIFACT='qwertycoin-gui-v2.0.5-macos-arm64')
            process = subprocess.run(['bash', '-c', script], cwd=directory, env=environment,
                                     text=True, capture_output=True, check=True)
            self.assertEqual(process.stderr, '')
            result = (directory / 'release-assets/RELEASE-NOTES.md').read_text()
            self.assertIn('`qwertycoin-gui-v2.0.5-windows-x86_64-setup.exe`', result)
            self.assertIn('`qwertycoin-gui-v2.0.5-windows-x86_64.zip`', result)
            self.assertIn('`qwertycoin-gui-v2.0.5-macos-arm64.dmg`', result)
            self.assertTrue(result.endswith(source_notes))
            self.assertIn('`no-go` (0/13)', result)


if __name__ == '__main__':
    unittest.main()
