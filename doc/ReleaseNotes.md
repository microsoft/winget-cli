## New in v1.29

Nothing yet.

## Bug Fixes

### Portable installer alias handling

Portable installs now preserve the original executable filename instead of renaming it when an alias is needed. For aliases requested through `--rename`, `Commands`, or `PortableCommandAlias`, WinGet creates a hardlink alias and keeps the original file as the source executable.

This change resolves alias failures in non-symlinked scenarios, including cases where WinGet adds the install directory to `PATH` instead of creating links. Because the alias is now created as an executable hardlink in the install location, command aliases remain available and consistent even when symlink creation is skipped.

### Minor Bug Fixes
* Fixed a crash (`0x8000ffff`) when using `--disable-interactivity` with the Resume experimental feature enabled during install operations.
