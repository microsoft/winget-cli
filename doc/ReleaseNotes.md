## New in v1.29

<!-- Nothing yet! -->

# Experimental Feature: 'listDetails'

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
