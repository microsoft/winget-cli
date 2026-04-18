## New in v1.29

# New Feature: NoInstaller manifest type

Manifests can now declare `InstallerType: noinstaller` (manifest version `1.29.0`). This type is intended for software a publisher no longer offers as a direct download, allowing winget to continue correlating against the installed package without providing an installer URL.

Key behaviours:

- **`winget install`** — immediately stops with the package's `InstallerAvailabilityMessage` if one is set, or a default "The installer for this package is no longer available." message otherwise. The exit code is `0x8A150116` (`APPINSTALLER_CLI_ERROR_INSTALLER_NOT_AVAILABLE`).
- **`winget show`** — displays `Installer Availability Message:` instead of `Installer Url:`, and shows `Offline Distribution Supported: false`.
- **`winget upgrade`** — a real installer is always preferred over a `noinstaller` entry when both exist for the same package. A `noinstaller` entry is only selected when it is the only applicable option, at which point the install flow blocks as above.
- **Manifest fields** — `InstallerUrl` must not be present. `InstallerSha256`, `ProductCode`, and `AppsAndFeaturesEntries` are all supported for package correlation. `InstallerAvailabilityMessage` (optional, max 512 characters) may be set at root or per-installer level.

# New Feature: Source Priority

> [!NOTE]
> Experimental under `sourcePriority`; defaulted to disabled.

With this feature, one can assign a numerical priority to sources when added or later through the `source edit`
command. Sources with higher priority are sorted first in the list of sources, which results in them getting put first
in the results if other things are equal.

> [!TIP]
> Search result ordering in winget is currently based on these values in this order:
> 1. Match quality (how well a valid field matches the search request)
> 2. Match field (which field was matched against the search request)
> 3. Source order (was always relevant, but with priority you can more easily affect this)

Beyond the ability to slightly affect the result ordering, commands that primarily target available packages
(largely `install`) will now prefer to use a single result from a source with higher priority rather than prompting for
disambiguation from the user. Said another way, if multiple sources return results but only one of those sources has
the highest priority value (and it returned only one result) then that package will be used rather than giving a
"multiple packages were found" error. This has been applied to both winget CLI and PowerShell module commands.

### REST result match criteria update

Along with the source priority change, the results from REST sources (like `msstore`) now attempt to correctly set the
match criteria that factor into the result ordering. This will prevent them from being sorted to the top automatically.

## Minor Features

### Preserve installer arguments across export and import

`winget export` now captures the `--override` and `--custom` arguments that were used when a package was originally installed and saves them into the export file. When subsequently running `winget import`, those values are automatically re-applied during installation — `--override` replaces all installer arguments and `--custom` appends extra switches — so packages can be reinstalled with the same customizations without any manual intervention. Both fields are optional and independent of each other; packages without stored installer arguments are unaffected.

### --no-progress flag

Added a new `--no-progress` command-line flag that disables all progress reporting (progress bars and spinners). This flag is universally available on all commands and takes precedence over the `visual.progressBar` setting. Useful for automation scenarios or when running WinGet in environments where progress output is undesirable.

### MCP `upgrade` support

The WinGet MCP server's existing tools have been extended with new parameters to support upgrade scenarios:

- **`find-winget-packages`** now accepts an `upgradeable` parameter (default: `false`). When set to `true`, it lists only installed packages that have available upgrades — equivalent to `winget upgrade`. The `query` parameter becomes optional in this mode, allowing it to filter results or be omitted to list all upgradeable packages. AI agents can use this to answer requests like "What apps can I update with WinGet?"
- **`install-winget-package`** now accepts an `upgradeOnly` parameter (default: `false`). When set to `true`, it only upgrades an already-installed package and returns a clear error if the package is not installed (pointing to `install-winget-package` without `upgradeOnly` instead). AI agents can use this to answer requests like "Update WinGetCreate" or, in combination with `find-winget-packages` with `upgradeable=true`, "Update all my apps."

### Authenticated GitHub API requests in PowerShell module

The PowerShell module now automatically uses `GH_TOKEN` or `GITHUB_TOKEN` environment variables to authenticate GitHub API requests. This significantly increases the GitHub API rate limit, preventing failures in CI/CD pipelines. Use `-Verbose` to see which token is being used.

### Improved `list` output when redirected

- `winget list` (and similar table commands) no longer truncates output when stdout is redirected to a file or variable — column widths are now computed from the full result set.
- Spinner and progress bar output are suppressed when no console is attached, keeping redirected output clean.

## Bug Fixes

* `winget export` now works when the destination path is a hidden file
* Fixed the `useLatest` property in the DSC v3 `Microsoft.WinGet/Package` resource schema to emit a boolean default (`false`) instead of the incorrect string `"false"`.
* `SignFile` in `WinGetSourceCreator` now supports an optional RFC 3161 timestamp server via the new `TimestampServer` property on the `Signature` model. When set, `signtool.exe` is called with `/tr <url> /td sha256`, embedding a countersignature timestamp so that signed packages remain valid after the signing certificate expires.
* File and directory paths passed to `signtool.exe` and `makeappx.exe` are now quoted, fixing failures when paths contain spaces.
* DSC export now correctly exports WinGet Admin Settings
