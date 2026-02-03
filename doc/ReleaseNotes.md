## New in v1.28

* Bumped the winget version to 1.28 to match the package version.
* Additional [options for limiting the size of log files](https://github.com/microsoft/winget-cli/blob/master/doc/Settings.md#file).

# New Feature: 'source edit'
New feature that adds an 'edit' subcommand to the 'source' command. This can be used to set an explicit source to be implicit and vice-versa. For example, with this feature you can make the 'winget-font' source an implicit source instead of explicit source.

To use the feature, try `winget source edit winget-font` to set the Explicit state to the default.

# New Experimental Feature: 'listDetails'

The new experimental feature `listDetails` enables a new option for the `list` command, `--details`.  When supplied, the output is no longer a table view of the results but is instead a series of `show` like outputs drawing data from the installed item.

An example output for a single installed package is:
```PowerShell
> wingetdev list Microsoft.VisualStudio.2022.Enterprise --details
Visual Studio Enterprise 2022 [Microsoft.VisualStudio.2022.Enterprise]
Version: 17.14.21 (November 2025)
Publisher: Microsoft Corporation
Local Identifier: ARP\Machine\X86\875fed29
Product Code: 875fed29
Installer Category: exe
Installed Scope: Machine
Installed Location: C:\Program Files\Microsoft Visual Studio\2022\Enterprise
Available Upgrades:
  winget [17.14.23]
```

If sixels are enabled and supported by the terminal, an icon for the installed package will be shown.

To enable this feature, add the 'listDetails' experimental feature to your settings.
```
"experimentalFeatures": {
    "listDetails": true
},
```

## Bug Fixes
* Portable Packages now use the correct directory separators regardless of which convention is used in the manifest
* `--suppress-initial-details` now works with `winget configure test`
* `--suppress-initial-details` no longer requires `--accept-configuration-agreements`
* Corrected property of `Font` experimental feature to accurately reflect `fonts` as the required setting value
