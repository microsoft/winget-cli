## New in v1.28

* Bumped the winget version to 1.28 to match the package version.
* Additional [options for limiting the size of log files](https://github.com/microsoft/winget-cli/blob/master/doc/Settings.md#file).

# Feature: 'source edit'
New feature that adds an 'edit' subcommand to the 'source' command. This can be used to set an explicit source to be implicit and vice-versa. For example, with this feature you can make the 'winget-font' source an implicit source instead of explicit source.

To use the feature, try `winget source edit winget-font` to set the Explicit state to the default.

## Bug Fixes
* Portable Packages now use the correct directory separators regardless of which convention is used in the manifest
* `--suppress-initial-details` now works with `winget configure test`
* `--suppress-initial-details` no longer requires `--accept-configuration-agreements`
* Corrected property of `Font` experimental feature to accurately reflect `fonts` as the required setting value
