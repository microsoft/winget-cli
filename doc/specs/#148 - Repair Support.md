---
author: Madhusudhan Gumbalapura Sudarshan @Madhusudhan-MSFT
created on: 2023-12-01
last updated: 2023-12-01
issue id: 148
---

# Repair Feature for Windows Package Manager

"For [#148](https://github.com/microsoft/winget-cli/issues/148)"

## Abstract

This specification outlines the design the design and implementation of a repair feature for the Windows Package Manager.The repair feature aims to provide a convenient and reliable way for users to fix any issues that may arise with their installed applications, such as corrupted files, missing dependencies, or broken registry entries.The initial implementation will support repair for main installer types, such as Msi, Wix, Msix and MSStore installer types, which have native repair capabilities from their respective frameworks. Additionally, other installer types such as Burn/Exe that can specify a custom repair switch in their YAML manifest files can also use the repair feature.The document also outlines the future plans for extending the repair feature to other installer types (Nullsoft, Inno, Portables) that do not have a standard repair mechanism.

## Inspiration

The motivation for creating a repair feature in the Windows Package Manager is to provide users with a convenient and consistent way to fix their malfunctioning applications, regardless of the installer type. Currently, users have to rely on different methods to repair their applications, such as using the Windows Settings app, the Control Panel, the command line, or the application's own repair tool. These methods are not always available, accessible, or intuitive, and they may vary depending on the application and the installer type. The repair feature aims to provide a unified experience for users to repair their applications, regardless of the installer type.

## Solution Design

### Repair Command Syntax
`winget repair <appname>`
will initiate the repair of the application whose name matches the search results.The command will display an error message if the application is not installed. The command will attempt to repair the application if it is installed. The command will display a success message if the repair succeeds. The command will display an error message if the repair fails.

### Repair Feature for different Installer Types with Native Repair Capabilities
  - Msi, Wix : The repair command will use the built-in repair features of the MSI installer type. It will run the msiexec command with the default [repair options](https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/msiexec#repair-options) to repair the application using a ShellExecute call or by invoking [MsiReinstallProduct](https://learn.microsoft.com/en-us/windows/win32/api/msi/nf-msi-msireinstallproductw) API call with `REINSTALLMODE` mode property set.
  - Msix : The repair command will use the built-in repair features of the MSIX installer type. It will make an MSIX API call to register the application package, which will usually fix the application as much as possible. 
    > We can't do the same thing as setting app repair for MSIX app right now because we can't use the private APIs that it uses to do repair operations with winget.
  - MSStore : The repair command will make an MSStore API  [StartProductInstallAsync](https://learn.microsoft.com/en-us/uwp/api/windows.applicationmodel.store.preview.installcontrol.appinstallmanager.startproductinstallasync?view=winrt-22621) call to repair the application with 'Repair' property of AppInstallOption set to true.

### Repair Feature for Installer Types that require Custom Repair Switch
  -  Burn/Exe : The custom switch for repair in the YAML manifest file will be used to perform the repair. The repair command will run the installer with the custom switch to fix the application. To have enough flexibility, different options are possible depending on the installer source used for repair. 
   - Installed Source: If the YAML manifest file has the 'ModifyRepair' switch, the repair command will use the modify command in the ARP 'ModifyPath' registry key with the repair switch through a ShellExecute call.
   - Remote Source: If the YAML manifest file has the 'InstallerRepair' switch, the repair command will get the matching installer from the remote source and use the repair switch on the downloaded installer through a ShellExecute call.
      > If neither switch is specified, the repair command will display an error message.

> Note: The initial implementation will not support repair for Nullsoft, Inno, Portables installer types. Based on feedback from the community, we may add the repair feature for these installer types in a future release.

## Manifest Changes
Addition of `ModifyRepair` property to InstallerSwitch
- The `ModifyRepair` property is used to define the custom repair option that works with ModifyPath ARP entry for those installer types that have ModifyPath ARP entry and allow repair through Modify flow.

Addition of `InstallerRepair` property to InstallerSwitch
-  With the `InstallerRepair` switch, the installer that matches the remote source is downloaded and then repaired by using ShellExecute to call the repair switch on the downloaded installer.

## Manifest Validation
- `ModifyRepair` and `InstallerRepair` switches are only valid for Burn/Exe installer types.
- `ModifyRepair` and `InstallerRepair` switches are mutually exclusive.

## Supported Repair Scenarios
- Repair for installed applications of Msi, Wix, Msix and MSStore installer types.
- Repair for the application using the custom repair switch specified in the YAML manifest file for Burn/Exe installer types.
  -  The ‘ModifyRepair’ switch when using the ModifyPath ARP entry for repair.
  -  The `InstallerRepair' switch when using the remote installer source for repair.

## Potential Issues
- Repair for Nullsoft, Inno, Portables installer types are not supported in the initial implementation.
- For Burn/Exe installer types 
  - the repair command will not work if the installer does not have a custom repair switch specified in the YAML manifest file.
  - the repair command will not work if the installed Burn/Exe installer type doesn't correlated to the remote installer type that has a custom repair switch specified in the YAML manifest file.

## Future considerations

- Repair for Nullsoft, Inno, Portables installer types. The possible options are:
   - Download matching installer , uninstall & Install or 
   - Download matching installer & re Install to overwrite existing files.
- Repair for Burn/Exe installer types without custom repair switch. The possible options are:
   - Download matching installer , uninstall & Install or 
   - Download matching installer & re Install to overwrite existing files.

## Resources
- https://learn.microsoft.com/en-us/windows/msix/desktop/managing-your-msix-reset-and-repair
- https://learn.microsoft.com/en-us/windows/win32/msi/reinstalling-a-feature-or-application
- https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/msiexec#repair-options
- https://learn.microsoft.com/en-us/uwp/api/windows.applicationmodel.store.preview.installcontrol.appinstallmanager.startproductinstallasync?view=winrt-22621
- https://learn.microsoft.com/en-us/windows/win32/api/msi/nf-msi-msireinstallproductw
