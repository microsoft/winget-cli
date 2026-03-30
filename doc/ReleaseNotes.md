## New in v1.29

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

### --no-progress flag

Added a new `--no-progress` command-line flag that disables all progress reporting (progress bars and spinners). This flag is universally available on all commands and takes precedence over the `visual.progressBar` setting. Useful for automation scenarios or when running WinGet in environments where progress output is undesirable.

### MCP `upgrade` support

The WinGet MCP server now exposes two new tools for upgrading packages:

- **`get-upgradable-winget-packages`** — Lists all installed packages that have available upgrades. Accepts an optional `query` parameter to filter by a specific package identifier, name, or moniker. AI agents can use this to answer requests like "What apps can I update with WinGet?"
- **`upgrade-winget-package`** — Upgrades a specific installed package to its latest available version. Returns a clear error if the package is not already installed (pointing to `install-winget-package` instead). Marked destructive, so MCP clients will prompt for confirmation before proceeding. AI agents can use this to answer requests like "Update WinGetCreate" or, in combination with `get-upgradable-winget-packages`, "Update all my apps."

### Authenticated GitHub API requests in PowerShell module

The PowerShell module now automatically uses `GH_TOKEN` or `GITHUB_TOKEN` environment variables to authenticate GitHub API requests. This significantly increases the GitHub API rate limit, preventing failures in CI/CD pipelines. Use `-Verbose` to see which token is being used.

## Bug Fixes

* `SignFile` in `WinGetSourceCreator` now supports an optional RFC 3161 timestamp server via the new `TimestampServer` property on the `Signature` model. When set, `signtool.exe` is called with `/tr <url> /td sha256`, embedding a countersignature timestamp so that signed packages remain valid after the signing certificate expires.
* File and directory paths passed to `signtool.exe` and `makeappx.exe` are now quoted, fixing failures when paths contain spaces.
