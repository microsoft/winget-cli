## New in v1.30

## New Features

### Source priority

Source priority is now available without enabling an experimental feature. Use `winget source add --priority <value>` or `winget source edit --name <source> --priority <value>` to configure it. Higher values take precedence; sources with equal priority still require disambiguation when multiple matches remain.

### Output locale override

Added a persistent `output.locale` setting to override winget interface language using a BCP47 tag.

Usage: add `"output": { "locale": "de-DE" }` to `settings.json`.

### `--ignore-unavailable` flag for `install`

Added a new `--ignore-unavailable` flag to the `install` command. When installing multiple packages, this flag allows the operation to continue with the remaining packages instead of failing entirely when one or more packages are not found in the configured sources. This brings the same behavior previously available with `import --ignore-unavailable` to direct multi-package installs.

## Bug Fixes

* Fixed an issue where `winget search --id <msstoreId>` could fail to return a Microsoft Store package unless `--exact` was also provided.
* Updated NUnit to v4
* Fixed a crash (`0x8000ffff`) when using `--disable-interactivity` with the Resume experimental feature enabled during install operations.
* Fixed relative path handling for rooted paths.
