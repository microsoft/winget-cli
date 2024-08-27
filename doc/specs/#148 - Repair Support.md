---
author: Madhusudhan Gumbalapura Sudarshan @Madhusudhan-MSFT
created on: 2023-12-01
last updated: 2024-08-27
issue id: 148
---

# Repair Feature for Windows Package Manager

"For [#148](https://github.com/microsoft/winget-cli/issues/148)"

## Abstract

This specification outlines the design and implementation of a repair feature for the Windows Package Manager.The repair feature aims to provide a convenient and reliable way for users to fix any issues that may arise with their installed applications, such as corrupted files, missing dependencies, or broken registry entries.The initial implementation will support repair for main installer types, such as Msi, Wix, Msix and MSStore installer types, which have native repair capabilities from their respective frameworks. Additionally, other installer types such as Burn/Exe/Nullsoft/Inno that can specify a custom repair switch in their YAML manifest files can also use the repair feature.The document also outlines the future plans for extending the repair feature to other installer types Portables that do not have a standard repair mechanism.

## Inspiration

The motivation for creating a repair feature in the Windows Package Manager is to provide users with a convenient and consistent way to fix their malfunctioning applications, regardless of the installer type. Currently, users have to rely on different methods to repair their applications, such as using the Windows Settings app, the Control Panel, the command line, or the application's own repair tool. These methods are not always available, accessible, or intuitive, and they may vary depending on the application and the installer type. The repair feature aims to provide a unified experience for users to repair their applications, regardless of the installer type.

## Solution Design

### Repair Command Syntax
`winget repair [[-q] <query>...] [<options>]`
will initiate the repair of the specified package. The command will display an error message if the application is not installed. The command will attempt to repair the application if it is installed. The command will display a success message if the repair succeeds. The command will display an error message if the repair fails.

#### Arguments
| Argument | Description |
|-------------|-------------|  
| **-q,--query**  |  The query used to search for an app. |


#### Optional Arguments

| Option | Description |
|--------|-------------|
| **-m, --manifest** | Must be followed by the path to the manifest (YAML) file.You can use the manifest to run the repair experience from a [local manifest file](#local-repair).|
| **--id** | Limits the repair to the specified ID of the application.|
| **--name** | Limits the repair to the specified name of the application.|
| **--moniker** | Limits the repair to the specified moniker of the application.|
| **-v --version** | Limits the repair to the specified version of the application.If not specified, the repair will be applied to the latest version of the application.|
| **--architectures** | Select the architecture |
| **-s --source** | Limits the search to specified source(s).Must be followed by the source name.|
| **-o, --log** | Directs the logging to a log file. You must provide a path to a file that you have the write rights to. |
| **-i --interactive** | Runs the repair in interactive mode.|
| **-h --silent** | Runs the repair in silent mode.|
| **-?, --help** | Get additional help on this command. |
| **--accept-source-agreements** | Accept all source agreements during source operations |
| **--logs, --open-logs** | Open the default logs location. |
| **--locale** | Sets the locale.|


### Repair Feature for different Installer Types with Native Repair Capabilities
  - Msi, Wix : The repair command will use the built-in repair features of the MSI installer type. It will run the msiexec command with the default [repair options](https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/msiexec#repair-options) to repair the application using a ShellExecute call.
  - Msix : The repair command will use the built-in repair features of the MSIX installer type. It will make an MSIX API call to register the application package, which will attempt to repair the package.
    > We can't do the same thing as setting app repair for MSIX app right now because we can't use the private APIs that it uses to do repair operations with winget.
  - MSStore : The repair command will make an MSStore API  [StartProductInstallAsync](https://learn.microsoft.com/en-us/uwp/api/windows.applicationmodel.store.preview.installcontrol.appinstallmanager.startproductinstallasync?view=winrt-22621) call to repair the application with 'Repair' property of AppInstallOption set to true.

### Repair Feature for Installer Types that require Custom Repair Switch
  - Burn, Exe, Nullsoft & Inno : The custom switch for repair in the YAML manifest file will be used to perform the repair. The repair command will run the installer with the custom switch to repair the application. To have enough flexibility, different options are possible depending on the installer source used for repair. 
   - Installed Source: 
     - If the YAML manifest file specifies the `Repair` switch and `Modify` as the `RepairBehavior`, the repair command will use the modify command in the ARP `ModifyPath` registry key, along with the repair switch, through a ShellExecute call, as long as `NoModify` and `NoRepair` ARP registry flags are not set to 1.
     - If the YAML manifest file specifies the `Repair` switch and `Uninstaller` as the RepairBehavior, the repair command will use the uninstall command in the ARP `UninstallString` registry key, along with the repair switch, through a ShellExecute call, as long as `NoRepair` APR registry flag is not set to 1.
   - Remote Source: If the YAML manifest file specifies the `Repair` switch and `Installer` value for the RepairBehavior, the repair command will obtain the matching installer from the remote source and use the repair switch on the downloaded installer through a ShellExecute call..

> If neither switch is specified, the repair command will display an error message.

> Note: The initial implementation will not support repair for Portables installer type. Based on feedback from the community, we may add the repair feature for these installer type in a future release.

## Manifest Changes
Addition of `Repair` property to InstallerSwitch
- The `Repair` property is used to set the custom repair option that works with `RepairBehavior` field that controls the different repair behavior.

Addition of `RepairBehavior` enumerable property to Installer Object
- With the `RepairBehavior` switch, we can adjust the repair behavior by choosing the installer source (Installed/local or remote) and making sure that the proper ARP registry entries are applied to identify the local installer type when carrying out a repair operation using a local installer source. 
- The permitted initial values for the `RepairBehavior` switch include:
   - Performing a repair using a Installed/Local Installer Source:
    - `Modify`: if this option is specified, the repair switch will be applied to the `ModifyPath` ARP command entry, as long as `NoModify` and `NoRepair` ARP registry flags are not set to 1.
    - `Uninstaller` : if this option is specified, the repair switch will be applied to the `UninstallString` ARP command entry, as long as `NoRepair` APR registry flag is note set to 1.
   - Performing a repair using a Remote Installer Source:
    - `Installer` : If this option is specified, the repair switch will be applied to the appropriate installer obtained from the remote installer source.

## Manifest Validation
-  Specifying `Repair` switch without `RepairBehavior` switch will result in an error.
 - Specifying `RepairBehavior` switch without `Repair` switch will result in an error.
 - `Repair` switch can't be empty when specified.
 - `RepairBehavior` switch can't be empty when specified.

## Handling Elevation Requirement
- The design presumes that the elevation requirements for modification/repair are consistent with those for installation, much like uninstallation.
- The expectations are:
  - If a non-elevated session tries to modify a package installed for machine scope, the installer should prompt for elevation, as observed in sample runs.
  - If a package installed for user scope attempts a repair operation in an admin context, it is restricted due to possible security risks.

## Supported Repair Scenarios
- Repair for installed applications of Msi, Wix, Msix and MSStore installer types.
- Repair for the application using the custom repair switch specified in the YAML manifest file for Burn/Exe/Nullsoft/Inno/Wix/Msi installer types.
  -  The appropriate repair behavior is determined by the combination of the `Repair` switch and the `RepairBehavior` value in the YAML manifest.

## Potential Issues
- For Msi based installer types
  - To repair an Msi-based installer using msiexec platform support, the original installer must be present at the location registered as 'InstallSource' in the ARP registry. If the original installer is moved or renamed, the repair operation will fail, which is consistent with the modify repair through the settings app. This issue is more likely with installers installed via the winget install command, as it removes the installer right after installation to reduce disk footprint by design, making the 'InstallSource' path invalid. However, this issue does not apply if the installer can store the installer in a package cache path for future use and registers that path in 'InstallSource' in the ARP registry at the time of installation.
- For Burn/Exe/Nullsoft/Inno installer types 
  - the repair command will not work if the installer does not have a custom repair switch specified in the YAML manifest file.
  - the repair command will not work if the installed Burn/Exe/Nullsoft/inno installer type doesn't correlate to the remote installer type that has a custom repair switch specified in the YAML manifest file.

## Future considerations

- Repair for Portables installer types. The possible options are:
   - Download matching installer , uninstall & Install OR
   - Download matching installer & re Install to overwrite existing files.
- Repair for Burn/Exe/Nullsoft/Inno installer types without custom repair switch. The possible options are:
   - Download matching installer , uninstall & Install OR 
   - Download matching installer & re Install to overwrite existing files.

## Resources
- https://learn.microsoft.com/en-us/windows/msix/desktop/managing-your-msix-reset-and-repair
- https://learn.microsoft.com/en-us/windows/win32/msi/reinstalling-a-feature-or-application
- https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/msiexec#repair-options
- https://learn.microsoft.com/en-us/uwp/api/windows.applicationmodel.store.preview.installcontrol.appinstallmanager.startproductinstallasync?view=winrt-22621
- https://learn.microsoft.com/en-us/windows/win32/api/msi/nf-msi-msireinstallproductw
