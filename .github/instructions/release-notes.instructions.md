---
applyTo: "doc/release-notes/**,doc/ReleaseNotes.md"
---

# Release note fragments

Follow `doc/release-notes/README.md`. Copy `doc/release-notes/template.md` into the
target release's `MAJOR.MINOR` directory and fill it out in a uniquely named file.
Determine the target version from `src/binver/binver/version.h`, not a hardcoded
version in these instructions. A fragment can contain both `## New Features` and
`## Bug Fixes`, with level-three titles and user-facing descriptions. Unused template
subsections are ignored and can be left unchanged, empty, or omitted.
For small fixes, add single-line bullets under `### Minor Bug Fixes` within `## Bug Fixes`.
The merger combines these lists across fragments and ignores unchanged template bullets.

Do not append entries to `doc/ReleaseNotes.md`, edit unrelated fragments, or commit
generated combined notes. Keep prior release directories and legacy archives intact.
When bumping the minor version, create the new release directory with `.gitkeep`.

Preview and validate with `.\doc\release-notes\Merge-ReleaseNotes.ps1`.
