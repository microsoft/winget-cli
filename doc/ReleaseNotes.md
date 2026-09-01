## New in v1.30

## New Features

### Pinning improvements

- `winget pin add` now supports an optional `--note` value for pin metadata.
- `winget pin add` now tracks the date that a pin was added or updated.
- `winget pin list` now supports an optional `--details` argument for showing extended pin data.
- The PowerShell pin cmdlets now expose the new pinning capabilities and metadata.

### `--ignore-unavailable` flag for `install`

Added a new `--ignore-unavailable` flag to the `install` command. When installing multiple packages, this flag allows the operation to continue with the remaining packages instead of failing entirely when one or more packages are not found in the configured sources. This brings the same behavior previously available with `import --ignore-unavailable` to direct multi-package installs.

## Bug Fixes

* Fixed an issue where `winget search --id <msstoreId>` could fail to return a Microsoft Store package unless `--exact` was also provided.
* Updated NUnit to v4
* Fixed a crash (`0x8000ffff`) when using `--disable-interactivity` with the Resume experimental feature enabled during install operations.
* Fixed relative path handling for rooted paths.
