# Release notes

Starting with 1.31, release notes are maintained as independent Markdown fragments to avoid
merge conflicts between unrelated PRs. Each release has a directory named `MAJOR.MINOR`,
such as `1.31`. Earlier releases remain in `..\release-notes-archive`.

## Adding a release note

1. Read `VERSION_MAJOR` and `VERSION_MINOR` in `src\binver\binver\version.h` to find the
   target release. For a backport, use the release targeted by that branch.
2. Copy `template.md` into that release's directory. Use a unique, descriptive filename,
   preferably `<issue-or-pr-number>-<short-description>.md`, or just a descriptive slug
   if no number is available. Do not edit another change's fragment.
3. Fill in `## New Features` for features and enhancements and `## Bug Fixes` for fixes.
   One fragment can contain both sections. Unused sections can be left unchanged, empty,
   or removed; subsections containing template placeholders are ignored.
4. Replace the title and description placeholders. Describe the user-visible change,
   with usage examples, limitations, or migration guidance where helpful. Use headings
   at level three or deeper inside the fragment. Use absolute URLs for links so they
   still work in the combined document.
   Fenced code examples may contain `#`, `##`, or `###` comments; they are preserved
   as example text, not interpreted as release-note headings. Backtick and tilde fences
   are supported.
   For small fixes, add one single-line bullet per fix under `### Minor Bug Fixes`
   in the `## Bug Fixes` section instead of writing a titled description.
5. Preview the combined notes with the script below. Commit the fragment, not the
   generated document. Internal refactoring, tests, and documentation-only changes
   generally do not need a release note.

For example, from the repository root:

```powershell
Copy-Item .\doc\release-notes\template.md .\doc\release-notes\1.31\1234-fix-package-selection.md
```

## Generating combined notes

```powershell
# Default: use the binary's current major/minor version.
.\doc\release-notes\Merge-ReleaseNotes.ps1

# Select a release explicitly and write the document outside the fragments directory.
.\doc\release-notes\Merge-ReleaseNotes.ps1 -Version 1.31 |
    Set-Content -LiteralPath "$env:TEMP\ReleaseNotes.1.31.md" -Encoding utf8
```

The script reads only `*.md` files directly in the selected release directory. It emits
`## New in v<version>`, then `New Features` and `Bug Fixes` in that order, omitting empty
categories. Sections from each fragment are collected into the matching category, ordered
by filename using ordinal sorting. Subsection bodies are preserved, apart from
leading/trailing whitespace and normalization to LF line endings. Template subsections
are omitted. All `### Minor Bug Fixes` lists are combined into one list at the end of
`## Bug Fixes`, in filename order. Both `*` and `-` bullets are accepted and emitted as
`*` bullets; unused template bullets are ignored without dropping completed fixes.
A release without completed notes produces `Nothing yet.`. Invalid fragments fail
generation with the filename in the error.

Do not place the generated document in a release directory: it is output, not a fragment.
`doc\ReleaseNotes.md` is only a stable pointer to these instructions.

## Starting a release

When bumping the binary minor version, create the matching directory here with a
`.gitkeep` file. Leave earlier release directories intact; do not copy their fragments
into the new release. Changes merged after a release branch was created belong to the
new target release, unless they are also intentionally backported.

## Validation

Run the merge script to validate the current release's fragments. It fails on invalid
categories, misplaced minor-fix lists, and minor fixes that are not single-line bullets.
Unchanged template sections are ignored rather than treated as errors:

```powershell
.\doc\release-notes\Merge-ReleaseNotes.ps1
```

The Azure build job runs this same command to validate the current release's fragments.
Changes under `doc\release-notes` trigger the pipeline even when no source files change.
