# Manual native release builds

The repository exposes two active GitHub Actions workflows:

```text
.github/workflows/release.yml
.github/workflows/assemble-release.yml
```

Both have only a `workflow_dispatch` trigger. Pushes, pull requests, tags and
GitHub releases do not start them. The normal build and Flatpak templates
remain under `.github/workflows-disabled/`.

## Scope

Each invocation builds exactly one target:

- `linux` — Linux x86_64 tarball built on Ubuntu 22.04 with a
  `GLIBC_2.35` compatibility ceiling;
- `windows` — Windows x86_64 zip;
- `macos` — macOS 15+ Apple Silicon tarball plus a drag-and-drop DMG,
  both containing the same verified `.app` bundle.

The request must supply the exact 40-character GUI commit SHA and a restricted
artifact prefix. Checkout and packaging fail unless the requested GUI revision,
the committed `qwertycoin/` Gitlink and the checked-out Core revision all agree.
The workflow checks the Core worktree again after the build so the build cannot
silently move or dirty the reviewed pin.

Compiler caches live in the checkout-local `.ccache` directory and are keyed by
platform and exact source revision. The workflow does not depend on a runner
context in job-level environment declarations.

Run targets sequentially so a platform failure can be reviewed before another
hosted build is started. A typical review sequence is Linux, Windows, then
macOS:

```sh
revision=$(git rev-parse HEAD)

gh workflow run release.yml --ref master \
  -f release_name=qwertycoin-gui-v2.0.1 \
  -f expected_revision="$revision" \
  -f target=linux
```

Repeat only after reviewing the completed run and change `target` to `windows`
or `macos`. Do not use a moving branch name as `expected_revision`.

## Artifact checks

`tools/release/package_artifacts.sh` rejects incomplete packages. It requires
the GUI, pinned daemon, wallet CLI, wallet RPC, brand assets, fonts and licenses.
It also validates the platform runtime:

- Linux recursively bundles non-glibc ELF dependencies, assigns relative
  RPATHs, proves that no non-system library resolves outside the package and
  rejects every ELF object that requires a glibc symbol newer than 2.35. Its
  required runtime set includes the Qt Labs Platform integration used by the
  native menu and file-dialog components. The packaged GUI must also load its
  complete QML root cleanly with the offscreen software renderer;
- Windows preserves the top-level Qt/dependency DLLs produced by `windeployqt`
  and requires the platform, SVG, Qt Quick and Qt Labs Platform QML modules;
- macOS requires the Qt frameworks, Cocoa/SVG plugins, Qt Quick and Qt Labs
  Platform QML modules produced by `macdeployqt`, rejects Homebrew/runner
  dependency paths and runtime search paths, verifies every bundled Mach-O is
  ARM64 with a deployment target no newer than macOS 15, and verifies the
  ad-hoc bundle signature. The deploy step also closes third-party transitive
  dylib dependencies inside `Contents/Frameworks`, and package verification
  rejects every remaining non-system dependency that cannot resolve inside the
  app. After that package check, the workflow uses the
  already packaged app bundle to create the DMG; it does not compile the wallet
  a second time.

Every package contains `BUILD-INFO.txt` with GUI/Core revisions, runner
platform and Qt version. Linux also records the enforced glibc ceiling; macOS
records its enforced minimum system version. The per-file SHA-256 manifest is
verified before the archive is uploaded. Uploaded review artifacts expire after
seven days.

The macOS review artifact contains exactly:

```text
<prefix>-macos-arm64.tar.gz
<prefix>-macos-arm64.sha256
<prefix>-macos-arm64.dmg
<prefix>-macos-arm64.dmg.sha256
```

The plain `.sha256` file remains the per-file manifest inside the tar archive.
It is not the DMG checksum. The `.dmg.sha256` file contains exactly one digest
for the exact DMG filename. The macOS job verifies the read-only image with
`hdiutil`, mounts it read-only, checks the `/Applications` link, background,
documentation, exact app contents and executable modes, and re-verifies the
bundle signature. It then copies the app to isolated storage, unmounts the DMG
and runs the copied GUI's QML smoke test so no mounted-image dependency can be
hidden.

### Create a DMG from an existing verified package

On macOS, an already extracted and independently verified package can be used
without CMake or a compiler:

```sh
python3 -m venv .venv-dmgbuild
.venv-dmgbuild/bin/pip install --require-hashes \
  -r tools/release/dmgbuild-requirements.txt

artifact=qwertycoin-gui-v2.0.2-macos-arm64
DMGBUILD_BIN="$PWD/.venv-dmgbuild/bin/dmgbuild" \
  tools/release/create_macos_dmg.sh "dist/$artifact" "$artifact" dist
tools/release/verify_macos_dmg.sh "dist/$artifact.dmg" "dist/$artifact"
```

The script refuses existing output paths. The version and architecture remain
part of the validated artifact name and are not hard-coded in the package
logic. The editable Finder background is
`tools/release/macos_dmg/background.svg`; its committed PNG is the render used
by `dmgbuild`. The volume is named **Qwertycoin Wallet** and contains
`Qwertycoin.app`, an `/Applications` link and a `Documentation` folder with the
original build identity, release notes, licenses and packaged brand sources.

For a visual native review, capture the Finder window after verification:

```sh
tools/release/capture_macos_dmg_layout.sh \
  "dist/$artifact.dmg" "dist/$artifact-finder.png"
```

## Signing and publication boundary

These jobs create native artifacts for review. They do not publish a release:

- Linux artifacts are unsigned;
- Windows artifacts are unsigned;
- macOS uses an ad-hoc signature only and is not notarized or stapled;
- no tag, GitHub Release, update metadata or public download is created.

For the current ad-hoc-signed build, Gatekeeper may block the first launch.
Attempt the launch once, then use **System Settings → Privacy & Security → Open
Anyway**. Do not remove quarantine metadata and do not disable Gatekeeper.

Developer ID signing, Apple notarization/stapling and Windows Authenticode are
separate release capabilities. Release notes, final checksums and publication
require a separate approved assembly step after inspection of the downloaded
artifacts. A published unsigned release must state these signing limits
prominently rather than implying publisher authentication.

## Release assembly

After all three downloaded candidates have been independently inspected, the
manual-only `assemble-release.yml` workflow can assemble them either into a
private draft prerelease or, with a separate explicit confirmation, a stable
public release. The workflow does not rebuild or modify a candidate. It
requires the exact candidate source revision and the three successful native
run IDs.

Before creating any release, it verifies that every referenced run:

- is a successful first-attempt `workflow_dispatch` execution of
  `release.yml` on `master`, using the same reviewed workflow definition as the
  assembler; the packaged `BUILD-INFO.txt` must independently bind the exact
  requested source revision;
- contains exactly one non-expired artifact with the expected platform name;
- contains archives with safe paths and links, no case-folding collisions,
  complete matching per-file SHA-256 coverage and exact GUI/Core build metadata;
- contains the required GUI, daemon, wallet CLI/RPC and platform runtime entry
  points;
- contains the exact four-file macOS candidate set above and a valid one-line
  checksum bound specifically to the expected DMG filename. Unexpected files,
  a missing DMG and mismatched digests are rejected.

It derives the allowed tag from the source tree's major/minor/revision values,
generates `SHA256SUMS` over the Linux tarball, Windows zip, macOS tarball and
macOS DMG, and targets the
immutable candidate commit. Existing tags or conflicting releases are
rejected. A rerun may retain and fully re-verify an exact matching release,
including the GitHub-computed digest of every asset.

The DMG is the preferred macOS download. Open it and drag **Qwertycoin** to
**Applications**. The macOS tarball remains available as an alternative.

`draft-prerelease` is the fail-safe default and requires `CREATE-DRAFT`. Stable
publication accepts only the exact source version tag (for example `v2.0.1`),
requires `release_kind=stable` and the literal `PUBLISH-STABLE` confirmation,
and still records the unsigned/ad-hoc signing boundary in the public notes.

Example for an already reviewed candidate set:

```sh
gh workflow run assemble-release.yml --ref master \
  -f release_tag=v2.0.2-rc1 \
  -f release_kind=draft-prerelease \
  -f expected_revision=<exact-40-character-gui-sha> \
  -f linux_run_id=<linux-run-id> \
  -f windows_run_id=<windows-run-id> \
  -f macos_run_id=<macos-run-id> \
  -f confirmation=CREATE-DRAFT
```

After the exact stable-labelled native archives have passed independent review,
the corresponding stable publication uses:

```sh
gh workflow run assemble-release.yml --ref master \
  -f release_tag=v2.0.1 \
  -f release_kind=stable \
  -f expected_revision=<exact-40-character-gui-sha> \
  -f linux_run_id=<linux-run-id> \
  -f windows_run_id=<windows-run-id> \
  -f macos_run_id=<macos-run-id> \
  -f confirmation=PUBLISH-STABLE
```
