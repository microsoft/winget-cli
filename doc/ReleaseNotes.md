## New in v1.30

## New Features

### Interactive package selection (experimental)

Set `experimentalFeatures.interactivePackageSelection` to `true` in settings to enable numbered choices when multiple packages match a single-package `install`, `show`, or `download` command in an interactive terminal. Enter a package number to continue or `0` to cancel.

This feature is disabled by default. Redirected and noninteractive callers retain the existing ambiguity error. Use `--id <ID> --exact --source <SOURCE>` to select a package explicitly, or `--disable-interactivity` to prevent prompts.

### Source priority

Source priority is now available without enabling an experimental feature. Use `winget source add --priority <value>` or `winget source edit --name <source> --priority <value>` to configure it. Higher values take precedence; sources with equal priority still require disambiguation when multiple matches remain.

### `--output-locale` argument

Added a new `--output-locale` argument that overrides the language used for WinGet's own output for a single invocation.
It is available on every command, accepts the same values as the `output.locale` setting (`en-US`, `de-DE`, `es-ES`, `fr-FR`, `it-IT`, `ja-JP`, `ko-KR`, `pt-BR`, `ru-RU`, `zh-CN`, `zh-TW`), and takes precedence over that setting.

`--output-locale` is independent of `--locale`: WinGet messages are shown in the locale requested by `--output-locale`, while manifest strings and package selection continue to use `--locale`.

Usage: `winget show <package> --locale zh-CN --output-locale de-DE`

### Output locale override

Added a persistent `output.locale` setting to override winget interface language using a BCP47 tag.

Usage: add `"output": { "locale": "de-DE" }` to `settings.json`.

### `--ignore-unavailable` flag for `install`

Added a new `--ignore-unavailable` flag to the `install` command. When installing multiple packages, this flag allows the operation to continue with the remaining packages instead of failing entirely when one or more packages are not found in the configured sources. This brings the same behavior previously available with `import --ignore-unavailable` to direct multi-package installs.

### COM install dependencies-only option

Added the contract version 30 `InstallOptions.InstallDependenciesOnly` property for COM callers that need to install a package's dependencies without installing the requested package.

## Bug Fixes

### Portable installer alias handling

Portable installs now preserve the original executable filename instead of renaming it when an alias is needed.
For aliases requested through `--rename`, `Commands`, or `PortableCommandAlias`, WinGet creates a hardlink alias and keeps the original file as the source executable.

This change resolves alias failures in non-symlinked scenarios, including cases where WinGet adds the install directory to `PATH` instead of creating links.
Because the alias is now created as an executable hardlink in the install location, command aliases remain available and consistent even when symlink creation is skipped.

### Minor Bug Fixes
* Fixed MSIX uninstall reporting success when package-family resolution finds no installed package.
* Fixed REST search results bypassing locally verifiable package filters and selectors. Extra manifests are retrieved only for complete source result sets of three or fewer packages. Normalized name/publisher criteria remain unvalidated client-side.
* Fixed installed-package matching incorrectly combining names and publishers from different manifest entries.
* Prevented unrestricted REST searches when a source declares all requested selectors unsupported.
* Prevented REST searches from looping indefinitely when continuation tokens repeat.
* Fixed Unicode case-insensitive prefix matching when case folding changes character lengths.
* Fixed an issue where `winget search --id <msstoreId>` could fail to return a Microsoft Store package unless `--exact` was also provided.
* Updated NUnit to v4
* Fixed a crash (`0x8000ffff`) when using `--disable-interactivity` with the Resume experimental feature enabled during install operations.
* Fixed relative path handling for rooted paths.
