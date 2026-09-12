# Manual release-candidate builds

The repository exposes one active GitHub Actions workflow:

```text
.github/workflows/release.yml
```

It has only a `workflow_dispatch` trigger. Pushes, pull requests, tags and
GitHub releases do not start it. The normal build and Flatpak templates remain
under `.github/workflows-disabled/`.

## Scope

Each invocation builds exactly one target:

- `linux` — Linux x86_64 tarball built on Ubuntu 22.04 with a
  `GLIBC_2.35` compatibility ceiling;
- `windows` — Windows x86_64 zip;
- `macos` — macOS Apple Silicon tarball containing the `.app` bundle.

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
  -f release_name=qwertycoin-gui-v2.0.0-rc1 \
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
  dependency paths and verifies the ad-hoc bundle signature.

Every package contains `BUILD-INFO.txt` with GUI/Core revisions, runner
platform and Qt version. Linux also records the enforced glibc ceiling. The
per-file SHA-256 manifest is verified before the archive is uploaded. Uploaded
review artifacts expire after seven days.

## Signing and publication boundary

These jobs create **release candidates for review**, not public releases:

- Linux artifacts are unsigned;
- Windows artifacts are unsigned;
- macOS uses an ad-hoc signature only and is not notarized or stapled;
- no tag, GitHub Release, update metadata or public download is created.

Developer ID signing, Apple notarization/stapling, Windows Authenticode,
release notes, final checksums and publication require a separate approved
release step after native launch and wallet smoke tests on the downloaded
artifacts.
