---
author: Roy MacLachlan RDMaclachlan/roy.maclachlan@microsoft.com
created on: 2023-02-09
last updated: 2023-02-09
issue id: 658
---

# Spec Title

"For [#658](https://github.com/microsoft/winget-cli/issues/658)"

## Abstract

This spec describes the functionality and high-level implementation design for the downloading of applications using the Windows Package Manager.

## Inspiration

This is inspired by customer feedback, and a need for broader application deployments:
* Customers want to share the installer with an offline device. 

## Solution Design

The Windows Package Manager will implement a new command `WinGet Download` that will offer customers with the ability to download a single package's installers. The new command will make use of the following optional parameter values:
* `--id <Identifier>`
* `--name <name>`
* `--moniker <moniker>`
* `--scope <scope>`
* `--locale <locale>`
* `--fileName <name>`
* `-v, --version <version>`
* `-s, --source <Source>`
* `-a, --architecture <architecture>`
* `-e, --exact`
* `-o, --output <path>`
* `-t, --type <installer type>`
* `--ignore-security-hash`
* `--accept-package-agreements`
* `--help`
* `--wait`
* `--verbose, --verbose-logs`

An update to the existing schema is required for the inclusion of the `<boolean> isDownloadable` property. If this property is set to `True` or is not specified, then the package's installer will be considered downloadable. If the property is set to `False`, then the request to download the package's installer will be refused.

Downloading the package's installer will still require that the package's installer hash be verified before becoming available to the user to interact with. The installer will be downloaded to the device with a `*.tmp` file extension that will be removed after the installer's hash has been validated. If the hash validation fails, the package's installer will be removed from the device.

When downloading the package's installer, if a file with the same name exists the new download will overwrite the existing file.

## UI/UX Design

### WinGet Command Line
Downloading an installer will output information relative to each step performed. Informing the user of any license agreements that must be accepted prior to download. Acceptance of license agreements will trigger the download to begin, displaying a progress bar that shows the download status. Upon download, the user will then be informed of the file hash validation status before being notified of the download status.

The following is representative of the user experience.

```PowerShell
PS C:\> WinGet Download Microsoft.visualstudiocode
Found Microsoft Visual Studio Code [Microsoft.VisualStudioCode] Version 1.73.1
This application is licensed to you by its owner.
Microsoft is not responsible for, nor does it grant any licenses to, third-party packages.
Starting package download...
   \
Successfully verified installer hash
Successfully downloaded

PS: C:\> 
```

When the user runs the Help action on the WinGet Download command, the following information will be provided:

```
PS C:\> WinGet Download --help
Windows Package Manager v1.1.1
Copyright (c) Microsoft Corporation. All rights reserved.

Downloads an identified package's installer, either found by searching a configured source or directly from a manifest. By default, the query must case-insensitively match the id, name, or moniker of the package. Other fields can be used by passing their appropriate option.

The following command alias are available

The following arguments are available:

The following options are available
    --id                            Filter results by id 
    --name                          Filter results by name
    --moniker                       Filter results by moniker
    --scope                         Select install scope (user or machine)
    --locale                        Locale to use (BCP47 format)
    --filename                      The value to rename the downloaded executable file to
    -v, --version                   Use the specified version; default is the latest version
    -s, --source                    Find package using the specified source
    -a, --architecture              Select the architecture to install
    -e, --exact                     Find package using exact match
    -o, --output                    Location to download file(s) to
    --installer-type                Specifies the filetype for the installer
    --ignore-security-hash          Ignore the installer hash check failure
    --accept-package-agreements     Accept all license agreements for packages
    --help                          Shows help about the selected command
    --wait                          |Prompts the user to press any key before existing
    --verbose, --verbose-logs       Enables verbose logging for WinGet

```

WinGet search will be used to enlighten customers of the package's installers downloadable status. the new column will only be displayed if the application has an installer can be downloaded.

```
PS C:\> winget search Microsoft.VisualStudioCode
Name                                  Id                                  Version Source Download
-------------------------------------------------------------------------------------------------
Microsoft Visual Studio Code          Microsoft.VisualStudioCode          1.75.0  winget True
Microsoft Visual Studio Code Insiders Microsoft.VisualStudioCode.Insiders 1.76.0  winget
```

### WinGet Setting - Default Download Output

A new setting will be included as the default output location for the user when package's installer is downloaded.

```PS
PS C:\> winget settings export
{"$schema":"https://aka.ms/winget-settings-export.schema.json","adminSettings":{"BypassCertificatePinningForMicrosoftStore":false,"InstallerHashOverride":false,"LocalArchiveMalwareScanOverride":false,"LocalManifestFiles":false},"userSettingsFile":"C:\\Users\\Roy\\AppData\\Local\\Packages\\Microsoft.DesktopAppInstaller_8wekyb3d8bbwe\\LocalState\\settings.json","userDefaultAppOutput:"C:\\Users\\Roy\\Downloads\\"}
```

The following items will be included in the WinGet Settings Export Schema 

```PS
"network: {
        "userDefaultAppOutput":"C:\\Users\\Roy\\Downloads\\"
        "useMeteredNetwork":"False"
}"
```

The "userDefaultAppOutput" setting will be used as the default folder used for outputting the package's installer to.

The "useMeteredNetwork" setting will accept boolean inputs, controlling the use of metered networks when downloading applications to the device.


### WinGet PowerShell Cmdlet
WinGet PowerShell cmdlet will download the identified package's installer based on the user specified parameters. While downloading the package's installer is occuring, PowerShell will show a progress bar displaying the progress. Afterwards, it will return an item object for the files downloaded.:

```PS
PS C:\> Get-WinGetPackage Microsoft.VisualStudioCode

    Directory: C:\Users\Roy\Downloads


Mode                 LastWriteTime         Length Name
----                 -------------         ------ ----
-a----        11/23/2021  10:05 AM         410253 Microsoft.VisualStudioCode.exe

```



```PS
PS C:\> Get-Help Get-WinGetPackage

NAME
    Get-WinGetPackage

SYNOPSIS
    Gets a package's installer or all installers from a WinGet configured source.

SYNTAX
    Get-WinGetPackage [-ID <System.String>] [-Name <System.String>] [-Moniker <System.String>] 
    [-Scope <System.String>] [-Locale <System.String>] [-FileName <System.String>] 
    [-Version <System.String>] [-Source <System.String>] [-Architecture <System.String>] [-Exact] 
    [-Output <System.String>] [InstallerType <System.String>] [-IgnoreSecurityHash] 
    [-AcceptPackageAgreement] [-Wait] [-Verbose]

DESCRIPTION
    Downloads an identified package's installer, either found by searching a configured source or 
    directly from a manifest. By default, the query must case-insensitively match the id, name, or 
    moniker of the package. Other fields can be used by passing their appropriate option.

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

* Modifications to how WinGet retrieves package's installers from the Package Manifest URLs could impact the functionality of the WinGet Install command.
* Update to the Schema will include the isDownloadable, this could impact services that use older versions of the schema that do not include the isDownloadable.
* Customers could overwrite an existing file with the same name as the downloaded file.

## Future considerations

* Downloading of UWP applications with install type: msstore.
* AAD Authentication

## Resources


