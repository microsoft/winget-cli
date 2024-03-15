---
author: Roy MacLachlan @RDMaclachlan, Ryan Fu @ryfu-msft
created on: 2023-02-09
last updated: 2024-02-15
issue id: 658
---

# `Download` command

"For [#658](https://github.com/microsoft/winget-cli/issues/658)"

## Abstract

This spec describes the functionality and high-level implementation design for downloading package installers using the Windows Package Manager.

## Inspiration

This is inspired by customer feedback, and a need for broader application deployments:
* Customers want to share the installer with an offline device.

## Solution Design

The `download` command will provide users with the ability to download any installer from a single package. The following command options are available:

```
  -d,--download-directory      Directory where the installers are downloaded to
  -m,--manifest                The path to the manifest of the package
  --id                         Filter results by id
  --name                       Filter results by name
  --moniker                    Filter results by moniker
  -v,--version                 Use the specified version; default is the latest version
  -s,--source                  Find package using the specified source
  --scope                      Select install scope (user or machine)
  -a,--architecture            Select the architecture
  --installer-type             Select the installer type
  -e,--exact                   Find package using exact match
  --locale                     Locale to use (BCP47 format)
  --ignore-security-hash       Ignore the installer hash check failure
  --skip-dependencies          Skips processing package dependencies and Windows features
  --header                     Optional Windows-Package-Manager REST source HTTP header
  --authentication-mode        Specify authentication window preference (silent, silentPreferred or interactive)
  --authentication-account     Specify the account to be used for authentication
  --accept-package-agreements  Accept all license agreements for packages
  --accept-source-agreements   Accept all source agreements during source operations
  -?,--help                    Shows help about the selected command
  --wait                       Prompts the user to press any key before exiting
  --logs,--open-logs           Open the default logs location
  --verbose,--verbose-logs     Enables verbose logging for winget
  --disable-interactivity      Disable interactive prompts
  ```

### Selecting the installer
A new command argument for `--installer-type` has been added to support selecting a specific installer type to download. A package installer should also be able to be selected by `--scope`, `--architecture`, and `--locale`.

### Downloading the installer
Downloading the package's installer will still require that the package's installer hash be verified before becoming available to the user to interact with. By default, installers will be downloaded to a unique folder name located in the `%USERPROFILE%/Downloads` directory. The default download directory can be modified in the user's settings. The unique folder name is comprised of the package identifier and package version. The installer will be comprised of the package identifier, package version, scope, architecture, and locale. This naming pattern ensures that the installer is unique and identifiable based on the installer filters applied:

> Example installer download path name: `%USER_PROFILE%\Downloads\Microsoft.PowerToys_0.78.0\PowerToys (Preview)_0.78.0_User_X64_burn_en-US.exe`

When downloading the package's installer, if a file with the same name exists the new download will overwrite the existing file.

### Downloading the manifest
Along with downloading the installer, a merged manifest will be generated and outputted in the same installer download directory. The naming of the file will be exactly the same as the installer except for the extension which will be `.yaml`. The manifest is useful for providing information about the installer such as scope, product code, installer switches, etc.

## UI/UX Design

### WinGet Command Line
Downloading an installer will output information relative to each step performed. Informing the user of any license agreements that must be accepted prior to download. Acceptance of license agreements will trigger the download to begin, displaying a progress bar that shows the download status. Upon download, the user will then be informed of the file hash validation status before being notified of the download status.

The following is representative of the user experience.

```PowerShell
PS C:\> WinGet download --id Microsoft.VisualStudioCode
Found Microsoft Visual Studio Code [Microsoft.VisualStudioCode] Version 1.73.1
This application is licensed to you by its owner.
Microsoft is not responsible for, nor does it grant any licenses to, third-party packages.
Starting package download...
   \
Successfully verified installer hash
Installer downloaded:%USERPROFILE%\Downloads\Microsoft.VisualStudioCode_1.86.1\Microsoft Visual Studio Code_1.86.1_User_X64_inno_en-US.exe
```

When the user runs `download` command with the `--help` argument, the following information will be provided:

```
PS C:\> WinGet Download --help
Windows Package Manager v1.1.1
Copyright (c) Microsoft Corporation. All rights reserved.

Downloads the installer from the selected package, either found by searching a configured source or directly from a manifest. By default, the query must case-insensitively match the id, name, or moniker of the package. Other fields can be used by passing their appropriate option. By default, download command will download the appropriate installer to the user's Downloads folder.

usage: winget download [[-q] <query>] [<options>]
```

### WinGet Setting - Default Download Output

The following items will be included in the WinGet Settings Schema

```json
"DownloadBehavior": {
        "defaultDownloadDirectory":"%USERPROFILE%/Downloads/"
}
```

The "defaultDownloadDirectory" setting will be used as the default folder where the package installer and manifest is downloaded to.

### WinGet PowerShell Cmdlet
WinGet PowerShell cmdlet will download the identified package's installer and manifest based on the user specified parameters. While downloading the package's installer, PowerShell will show a progress bar displaying the progress. Once the download is complete, the status of the download will be shown to the user, along with the id, name, and source of the package.

```
C:\> Export-WinGetPackage -?

NAME
    Export-WinGetPackage

SYNTAX
    Export-WinGetPackage [[-Query] <string[]>] [-DownloadDirectory <string>] [-AllowHashMismatch] [-Architecture
    {Default | X86 | Arm | X64 | Arm64}] [-InstallerType {Default | Inno | Wix | Msi | Nullsoft | Zip | Msix | Exe |
    Burn | MSStore | Portable}] [-Locale <string>] [-Scope {Any | User | System | UserOrUnknown | SystemOrUnknown}]
    [-SkipDependencies] [-Version <string>] [-Id <string>] [-Name <string>] [-Moniker <string>] [-Source <string>]
    [-MatchOption {Equals | EqualsCaseInsensitive | StartsWithCaseInsensitive | ContainsCaseInsensitive}] [-WhatIf]
    [-Confirm] [<CommonParameters>]

    Export-WinGetPackage [[-PSCatalogPackage] <PSCatalogPackage>] [-DownloadDirectory <string>] [-AllowHashMismatch]
    [-Architecture {Default | X86 | Arm | X64 | Arm64}] [-InstallerType {Default | Inno | Wix | Msi | Nullsoft | Zip |
    Msix | Exe | Burn | MSStore | Portable}] [-Locale <string>] [-Scope {Any | User | System | UserOrUnknown |
    SystemOrUnknown}] [-SkipDependencies] [-Version <string>] [-WhatIf] [-Confirm] [<CommonParameters>]


ALIASES
    None


REMARKS
    None
```

## Capabilities

### Accessibility

Accessibility should not be impacted by this change. There will be a new column in WinGet search that will appear if an application is downloadable.

### Security

Security of the Windows Package Manager should not be impacted by this change.

### Reliability

There will be no change to the reliability of the Windows Package Manager.

### Compatibility

There will be no breaking changes to the code. A subsection of the WinGet Install functionality will be leveraged for this new functionality.

### Performance, Power, and Efficiency

## Potential Issues

## Future considerations

* AAD Authentication

## Resources
