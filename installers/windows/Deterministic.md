# Building the Windows Installer Reproducibly

The Qwertycoin Windows installer uses Inno Setup. Reproducible candidates
require every builder to use:

- the same reviewed GUI and Core commit pair;
- the same Windows artifact SHA-256 manifest;
- the same Inno Setup version;
- identical source-file timestamps from the artifact archive; and
- the unchanged `Qwertycoin.iss` definition.

The source artifact must contain the GUI, `qwertycoind`, wallet CLI, wallet RPC,
Qt libraries, QML plugins, SVG/image plugins, fonts, icons, README and license.
The installer definition preserves source timestamps with
`TimeStampsInUTC=yes` so builder time zones do not change the output.

Builders compare SHA-256 hashes of the unsigned installer. Code signing changes
the final file and is a separate release step. A reproducible cross-build does
not replace a native Windows start and wallet smoke test.
