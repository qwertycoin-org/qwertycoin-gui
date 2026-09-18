#!/usr/bin/env python3

import os
import pathlib
import subprocess
import tempfile
import textwrap
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[3]
SCRIPT = ROOT / "tools" / "release" / "complete_macos_runtime.sh"


class CompleteMacosRuntimeTests(unittest.TestCase):
    def test_copies_and_rewrites_framework_dependency(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = pathlib.Path(tmp)
            bundle = tmp_path / "Qwertycoin.app"
            executable = bundle / "Contents" / "MacOS" / "qwertycoin-gui"
            executable.parent.mkdir(parents=True)
            executable.write_bytes(b"mock executable")

            search_root = tmp_path / "qt" / "lib"
            framework_binary = (
                search_root
                / "QtSvg.framework"
                / "Versions"
                / "5"
                / "QtSvg"
            )
            framework_binary.parent.mkdir(parents=True)
            framework_binary.write_bytes(b"mock framework")

            mock_bin = tmp_path / "bin"
            mock_bin.mkdir()
            state = tmp_path / "rewritten"
            dependency = str(framework_binary)
            replacement = (
                "@executable_path/../Frameworks/"
                "QtSvg.framework/Versions/5/QtSvg"
            )

            (mock_bin / "file").write_text(
                "#!/usr/bin/env bash\necho 'Mach-O 64-bit dynamically linked'\n",
                encoding="utf-8",
            )
            (mock_bin / "otool").write_text(
                textwrap.dedent(
                    f"""\
                    #!/usr/bin/env bash
                    if [[ $1 == -D ]]; then
                      echo "$2"
                      if [[ $2 == *QtSvg.framework* ]]; then echo "{dependency}"; fi
                    elif [[ $1 == -L ]]; then
                      echo "$2:"
                      if [[ $2 == *qwertycoin-gui ]]; then
                        if [[ -e "$MOCK_STATE" ]]; then
                          echo $'\\t{replacement} (compatibility version 5.0.0, current version 5.15.19)'
                        else
                          echo $'\\t{dependency} (compatibility version 5.0.0, current version 5.15.19)'
                        fi
                      elif [[ $2 == *QtSvg.framework* ]]; then
                        echo $'\\t{dependency} (compatibility version 5.0.0, current version 5.15.19)'
                      fi
                      echo $'\\t/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1345.120.2)'
                    elif [[ $1 == -l ]]; then
                      exit 0
                    fi
                    """
                ),
                encoding="utf-8",
            )
            (mock_bin / "install_name_tool").write_text(
                "#!/usr/bin/env bash\n"
                "if [[ $1 == -change ]]; then touch \"$MOCK_STATE\"; fi\n",
                encoding="utf-8",
            )
            for tool in ("file", "otool", "install_name_tool"):
                (mock_bin / tool).chmod(0o755)

            env = os.environ.copy()
            env["PATH"] = f"{mock_bin}:{env['PATH']}"
            env["MOCK_STATE"] = str(state)
            result = subprocess.run(
                [str(SCRIPT), str(bundle), str(search_root)],
                check=False,
                capture_output=True,
                text=True,
                env=env,
            )

            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertTrue(
                (
                    bundle
                    / "Contents"
                    / "Frameworks"
                    / "QtSvg.framework"
                    / "Versions"
                    / "5"
                    / "QtSvg"
                ).is_file()
            )
            self.assertTrue(state.is_file())
            self.assertIn("Bundled transitive framework: QtSvg.framework", result.stdout)
            self.assertIn("macOS runtime closure verified", result.stdout)


if __name__ == "__main__":
    unittest.main()
