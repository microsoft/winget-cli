# Windows Package Manager PowerShell Modules

## History
This set of PowerShell modules was originally built as a Microsoft Hackathon project in 2021.

We started this project as an exploration to design the right set of cmdlets with PowerShell approved nouns and verbs. As we began to explore wrapping the Windows Package Manager we identified several areas of complexity. Rather than slow down progress on building PowerShell cmdlets, we decided to forge ahead and call out the areas where the experience with pipelines is sub-optimal.

For example, the Windows Package Manager (later referenced as just CLI) CLI was designed for displaying output in the standard width Windows Terminal. As such, long package names and "Id"s are truncated with a single width ellipsis character. Users wanting to use the PowerShell pipeline to pass values to another cmdlet will likely encounter undesired behavior due to this truncation. We have decided to declare the "Microsoft.WinGet" module as an alpha release until these problems have been resolved. We are planning to change the status of "PreRelease" to beta once we believe we have addressed the issues related to piping the output to other cmdlets.

The module and associated cmdlets were essentially handcrafted as the Hackathon team came up to speed with PowerShell idioms. The module and associated cmdlets in the Microsoft.WinGet.Client/crescendo directory were crafted using the "Microsoft.PowerShell.Crescendo" module. Several manual changes were applied to these files as well, but the idea was to leverage crescendo to speed up development.

As we continued, we also identified work being done in support of private REST sources. We decided to create separate modules to speed up development in that are of the product as well as standardizing some of the module work.

The "Microsoft.WinGet" module is a top level module to organize the others. They child modules are "Microsoft.WinGet.Client" intended to represent the [native PowerShell module](https://github.com/microsoft/winget-cli/issues/221) feature request. The "Microsoft.WinGet.Create" module in the [winget-cli-restsource repository](https://github.com/microsoft/winget-cli-restsource) is expected to support building and modifying manifests in private sources (related to the Windows Package Manager Manifest Creator [REST support feature](https://github.com/microsoft/winget-create/issues/3)). The third module "Microsoft.WinGet.Source" is intended to simplify working the private REST sources like the [reference implementation](https://github.com/microsoft/winget-cli-restsource). A fourth module may be created for the [winget create tool](https://github.com/microsoft/wingetcreate).

## Terms and Conventions used

### Package
The term "package" is used to reference an application or program. For example, Windows Terminal is a package. It's identifier "Microsoft.WindowsTerminal" is available in the [Windows Package Manager Community App Repository](https://github.com/microsoft/winget-pkgs).


### Manifest 
The term "manifest" is used to reference the metadata about an application or program. In the [Windows Package Manager Community App Repository](https://github.com/microsoft/winget-pkgs) manifests are represented as YAML files. In a REST source, manifests are represented as JSON structured data. For the sake of transparency, the manifests downloaded by the Windows Package Manager from the default "**winget**" source, the manifests are taken from the GitHub repository and merged into a single YAML file per package version.


### Conventions
We attempted to make use of the approved verbs for PowerShell. In areas where the verb may have been contentious, we decided to at least be consistent with ourselves. The Open verb is used to open a directory in file explorer (Windows Package Manager Manifest Creator installer cache). The Edit verb is used to open the settings.json file for the Windows Package Manager Manifest Creator and the Windows Package Manager.

### Status
The modules should be treated as experimental at this stage of development. They are essentially calling the Windows Package Manager executable and attempting to parse text output that wasn't designed for PowerShell.

### Future
We expect to enhance the COM interface to support JSON output in the future so the client module can provide rich PowerShell objects.

---
>Drafted in October and preserved for context.

## WinGet Modules
* Microsoft.Winget
* Microsoft.WinGet.Client
* Microsoft.WinGet.Create
* Microsoft.WinGet.Source

## Microsoft.WinGet.Client cmdlets 

### Add-WinGetSource
Adds a source for the Windows Package Manager to use

### Disable-WinGetLocalManifest
>This must be run in administrator mode

### Edit-WinGetClientSetting
Open Windows Package Manager settings file

### Enable-WinGetLocalManifest
>This must be run in administrator mode

### Find-WinGetPackage
Searches for packages in configured sources

### Get-WinGetPackage
Displays the list of packages installed on the local system

### Get-WinGetSource
Displays the list of sources configured for the Windows Package Manager

### Get-WinGetVersion
Gets the version for the Windows Package Manager Manifest Creator

### Install-WinGetPackage
Installs the given package

### Remove-WinGetSource
Removes a configured source from the Windows Package Manager

### Reset-WinGetSource
Resets the default sources for the Windows Package Manager

### Uninstall-WinGetPackage
Uninstalls a package from the local system

### Upgrade-WinGetPackage
Upgrades a package installed on the local system

### Get-WinGetInstaller (ToDo)
Displays the installer Install-WinGetPackage would select for the local system

---
## Microsoft.WinGet.Create Module cmdlets

### Get-WinGetCreateVersion (ToDo)
Gets the version for the Windows Package Manager Manifest Creator

### Edit-WinGetCreateSetting (ToDo)
Open Windows Package Manager Manifest Creator settings file

### New-WinGetManifest -Path (ToDo)
Creates a new Manifest

### Set-WinGetManifest - Path (ToDo)
Updates fields of an existing manifest

### Submit-WinGetManifest -Path (ToDo)
Submits a manifest to the Windows Package Manager App Repository for validation

### Add-WinGetManifestVersion (ToDo)
Adds a version to an existing manifest

### Test-WinGetManifest (ToDo)
Validates a manifest

### Open-WinGetCreateCache (ToDo)
Opens the cache folder storing the downloaded installers

### Get-WinGetCreateCache (ToDo)
Lists out all the downloaded installers stored in cache

### Clear-WinGetCreateCache (ToDo)
Deletes all downloaded installers in the cache folder

---
## Microsoft.WinGet.Source Module cmdlets

** Focus on Private Repository (Rest), If there is time.. then we can look at future support of Public WinGet Source.

### Add-WinGetManifest 
Example: Add-WinGetManifest -Path "C:\Folder\File.json" -Source PrivateRepo
* Submit a Manifest to a repository
* -Version: Returns only a specific version of the Manifest as a manifest.
* If Version not specified, then return the latest version only as part of the manifest.

### Get-WinGetManifest
Example: Get-WinGetManifest -Id Microsoft.PowerToys -Source PrivateRepo
* Gets a Manifest
* Needs to have the returned results "Beautified" when shown to the screen.

### Set-WinGetManifestVersion
Set-WinGetManifestVersion [-Id] Microsoft.PowerToys [--Source] PrivateRepo --ShortDescription "New Description" [--Version] "33.0.0.0" 
--Source {(PrivateRepo)}

### Add-WinGetManifestVersion
 Does not overwrite previously existing values. User must run "Set" to modify.

### Get-WinGetManifestVersion
Example: Get-WinGetManifestVersion [-Id] Microsoft.PowerToys -Version 3.0.0.0
Returns the values of a specific version in a Manifest

### Remove-WinGetManifestVersion
This will remove manifest versions from a manifest located in the Private Source only.

### Remove-WinGetManifest
This will remove manifests from the Private Source only.

---

## ToDo

This is not an exhaustive list, but is here as guide for work that needs to be performed.

* Localization Support
* Handle error messages from the client in several scenarios
* Handle Group Policy messages
* Header is only valid in the context of a single source parameter. We may want to validate in the cmdlet
* Validation needs to be built
* The Modules should be moved to a new GitHub repository patterned after [Crescendo](https://www.powershellgallery.com/packages/PSPackageProject/0.1.18)
* Ultimately, the child modules will be moved to their respective GitHub repositories and the parent will stay in it's own repository.
* Validation should be implemented
* A CI Pipeline should be built to sign and publish the modules to the PowerShell gallery
* Packaging with [PSPackageProject](https://www.powershellgallery.com/packages/PSPackageProject/0.1.18)
* Documentation with [platyPS](https://www.powershellgallery.com/packages/platyPS/0.14.2)
* Support changes from https://github.com/microsoft/winget-cli/issues/1597

---

## Parking Lot

PowerShell modules should provide at least parity with the Windows Package Manager

The CLI is inefficient at returning multiple versions / installers. If we expand beyond what it can do, when we inevitably switch over to the CLI as the tool for running, then the community will see this as a loss of functionality.