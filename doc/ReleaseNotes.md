## New in v1.30

## New Features

### Pinning improvements

- `winget pin add` now supports an optional `--note` value for pin metadata.
- `winget pin add` now tracks the date that a pin was added or updated.
- `winget pin list` now supports an optional `--details` argument for showing extended pin data.
- The PowerShell pin cmdlets now expose the new pinning capabilities and metadata.

## Bug Fixes

* Updated NUnit to v4
* Fixed a crash (`0x8000ffff`) when using `--disable-interactivity` with the Resume experimental feature enabled during install operations.
