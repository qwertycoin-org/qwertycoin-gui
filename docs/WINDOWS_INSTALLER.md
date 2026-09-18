# Windows installer, upgrades and user-data boundary

The Windows release job creates two distributions from the **same verified
Windows package**:

- the portable `<prefix>-windows-x86_64.zip`; and
- the all-users `<prefix>-windows-x86_64-setup.exe` installer.

The installer does not compile or download any GUI, Core, Qt, QML, font or
runtime component. `tools/release/windows/create_windows_installer.ps1` first
builds a case-insensitive, path-safe SHA-256 manifest of the already packaged
program tree. It then generates one explicit Inno Setup `[Files]` entry per
manifested file and compiles `tools/release/windows/Qwertycoin.iss` with the
pinned Inno Setup version installed by the workflow.

## Installation

The default all-users destination is the native 64-bit Programs location under
`Qwertycoin`. Setup requests normal UAC elevation for installation only. The
wallet itself, its shortcuts and the bundled daemon run as the invoking regular
user and do not need administrator rights.

Setup creates a Start menu shortcut. A desktop shortcut is selected on the
first installation and may be deselected; the choice is retained on upgrades.
Both shortcuts target `qwertycoin-gui.exe` and set the working directory to the
installation directory. Setup does not automatically start the elevated copy
of the wallet at the end. Start it through either shortcut after Setup exits.

English and German installer languages are included. The original Qwertycoin
application icon is used for Setup, shortcuts and the Installed Apps entry.

## Actual Windows data locations

These locations come from the current application and embedded Core code; the
installer does not replace them with hard-coded profile paths:

- the GUI's default wallet directory is Qt's Windows
  `QStandardPaths::DocumentsLocation` plus `Qwertycoin\wallets`. A redirected
  Documents folder, including OneDrive redirection, therefore remains in use;
- normal GUI settings use per-user `QSettings` storage for organization
  `qwertycoin-project` and application `qwertycoin-gui`;
- portable settings exist only when
  `<working-directory>\qwertycoin-storage\settings.ini` already exists. The
  installer never ships that active file or scans/migrates portable copies;
- the daemon continues to use Core's default data directory or an explicitly
  configured `--data-dir`;
- the EPoSE service keystore remains under the configured/default Core data
  directory at `epose-v2\<network>\service-keystore-v2`.

A wallet from a previous portable ZIP can still be selected through the
existing Open Wallet dialog. The ZIP directory and its settings are not deleted
or migrated automatically.

## Upgrade and repair safety

The installer has one stable, version-independent AppId and reuses the previous
installer directory. It rejects a downgrade, while reinstalling the same
version is a repair.

Before the first change, Setup validates:

1. the embedded new program manifest and its compiled SHA-256 digest;
2. the previous installed manifest against the protected machine-wide digest;
3. every relative path, case-insensitive uniqueness and directory boundary;
4. that no target component is a junction, symbolic link or other reparse
   point; and
5. that every existing new-package target was owned by the previous installer.

A first installation into a non-empty unknown directory is refused. On an
upgrade, an unknown user file whose name collides with a new package file is
also refused before changes begin; it is never silently overwritten.

All current private application files use Inno Setup's `ignoreversion` rule, so
repair and upgrade replace them even if embedded file versions did not change.
After copying, Setup recalculates every installed SHA-256 digest before it
removes anything obsolete.

Only paths in the authenticated previous program manifest can become obsolete.
Existing obsolete files are renamed into an installer-private rollback area
after the new program set has been installed and verified. They are restored if
Setup fails and deleted only after successful completion. A persistent journal
allows the next invocation to resolve an interrupted cleanup without guessing.
There is no recursive purge of the installation directory.

Restart Manager is enabled for package-owned files. Setup requests regular
application closure and never runs a blanket `taskkill /F`. If files remain
locked, the update fails instead of silently leaving a mixed old/new program
set.

## Data retained by update and uninstall

Updates, repairs and uninstall never enumerate or delete wallet/profile data.
In particular they leave these classes untouched, wherever configured:

- wallet files, key files, caches, backups and exports, including extensionless
  names;
- normal and portable settings;
- blockchain databases and custom daemon data directories;
- EPoSE keystores and node identities; and
- unknown files manually placed in the installation directory.

Uninstall removes Inno-managed program files, shortcuts, installer metadata and
the Installed Apps entry. Unknown files keep the directory non-empty and are
left in place. There is intentionally no “delete user data” option.

## Local packaging

On Windows, package an already verified and extracted Windows release tree
without CMake or a compiler:

```powershell
$artifact = 'qwertycoin-gui-v2.0.2-windows-x86_64'
tools/release/windows/create_windows_installer.ps1 `
  -PackageDir "dist/$artifact" `
  -ArtifactName $artifact `
  -Version '2.0.2' `
  -OutputDir dist `
  -IsccPath 'C:\Program Files\Inno Setup 7\ISCC.exe'
```

The package must contain matching `BUILD-INFO.txt` source, Core, application
version, Windows and architecture fields. Existing setup outputs are refused.
The result is the setup executable plus a one-line, filename-bound
`.setup.exe.sha256` file. The portable ZIP's plain `.sha256` remains its
per-file package manifest and is not reused as the installer checksum.

## Signing boundary

The current Windows ZIP and installer are unsigned. Their SHA-256 digests prove
downloaded bytes, not publisher identity, and Windows SmartScreen may warn.
Do not disable SmartScreen or Defender. Authenticode signing is a separate
release capability; final checksums must be generated only after any future
signing step.
