## New in v1.29

## New Features

### Output locale override

Added a persistent `output.locale` setting to override winget interface language using a BCP47 tag.

Usage: add `"output": { "locale": "de-DE" }` to `settings.json`.

## Bug Fixes

* Fixed a crash (`0x8000ffff`) when using `--disable-interactivity` with the Resume experimental feature enabled during install operations.
