## New in v1.12
* MCP server available; run `winget mcp` for assistance on configuring your client.
* App Installer now uses WinUI 3. The package dependency on WinUI 2 has been replaced by a dependency on the Windows App Runtime 1.8.
* Manifest schema and validation updated to v1.12. This version update adds `Font` as an `InstallerType` and `NestedInstallerType`.

## Bug Fixes
* Manifest validation no longer fails using `UTF-8 BOM` encoding when the schema header is on the first line
* Upgrading a portable package with dev mode disabled will no longer remove the package from the PATH variable.
* Fixed source open failure when there were multiple sources but less than two non-explicit sources.

## Experimental Features
* Experimental support for Fonts

---
### Experimental support for Fonts
The following snippet enables experimental support for fonts via `winget settings`. The `winget font list` command will list installed font families and the number of installed font faces.
```JSON
{
  "$schema" "https://aka.ms/winget-settings.schema.json",
  "experimentalFeatures": {
    "fonts": true
  }
}
```
Manifest Schema version updated to 1.12.0 to add support for fonts.

Experimental initial Font Install and Uninstall via manifest for user and machine scopes has been added.

The font 'list' command has been updated with a new '--details' feature for an alternate view of the installed fonts.
