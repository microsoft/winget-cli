## New in v1.29

Nothing yet.

## Bug Fixes

* Refactored source type handling to use a `SourceType` enum with centralized string conversion, and added stronger `source add` type validation (#4463).
* Fixed a crash (`0x8000ffff`) when using `--disable-interactivity` with the Resume experimental feature enabled during install operations.
