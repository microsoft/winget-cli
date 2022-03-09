---
author: Demitrius Nelon @denelon
created on: 2022-03-09
last updated: 2022-03-09
issue id: 182
---

# Portable/Standalone applications

"For [#182](https://github.com/microsoft/winget-cli/issues/182)"

## Abstract

Several packages do not have an installer. They are simply a binary executable. This specification describes how these types of programs will be treated as "packages" by the Windows Package Manager.

## Inspiration

Just because an installer doesn't exist for a particular package doesn't mean you shouldn't be able to install it with the Windows Package Manager.

## Solution Design

The installer type “portable” will be added to the enumerated list of installer types to support these types of packages.

When one of these packages is encountered, there are several aspects to “installing” these packages and supporting the “upgrade” behavior. Notable concerns include the location of the package, creating or updating an entry in Windows Apps & Features.

### Command Line Arguments

Users should be able to specify the location for where the program is “installed” on their machine with the “--location” argument.

Some packages like GitLab Runner have a file name including extra metadata (gitlab-runner-windows-386.exe). The installation instructions suggest renaming the file after it has been downloaded. In that vein, portable / standalone executables should have their “command” value specified so the Windows Package Manager can perform a default rename behavior. In addition, a “--rename” argument should be added so the user can choose a value they prefer. To have the upgrade scenario honor the custom name, this information should be recorded in the installed packages data store.

Some portable applications generate or consume other files. An additional argument "--purge" will be added for the uninstall scenario to remove all files and subsequently the directory if the user wishes. The corollary argument "--preserve" will be used to preserve files if the default setting has been modified.

### Settings

Users should have settings to be able to specify a new default location other than the system default for the Windows Package Manager.

The default path for installing these packages is "%LOCALAPPDATA%/WinGet_Packages/User" for user based installs. The default path for installing these packages is "%LOCALAPPDATA%/WinGet_Packages/Machine"for machine wide installs.

The corresponding settings are "PortablePackageUserRoot" and "PortablePackageUserRoot".

>Note: The "packageIdentifier" will be used to generate subdirectories for these binaries to be installed to.

Using Gitlab.gitlab runner as an example would result in the .exe being placed in "%LOCALAPPDATA%/WinGet_Packages/User/Gitlab.gitlab-runner/"
If a user has configured a path to be used for portable applications, that path should be honored. The user should also be able to specify if they want all portable packages placed in the same directory, or in a directory per package.

A related setting for the uninstall scenario will be able to specify the default behavior for either "--clean" or "--purge".

### Install

When the portable application is being installed by the Windows Package Manager, an entry will be created in Windows Apps & Features so the user will be able to see that the application is installed.

The "AppPath" registry subkey will identify the path to the application so the user will be able to execute the program from any path via command prompt.

The data from “AppsAndFeaturesEntry” in the manifest will be used to specify the name of the package in Windows Apps & Features.  Constraints for manifests with portable packages are covered below in the section on validating manifests. If the “AppsAndFeaturesEntry” doesn’t have a “DisplayName” value, then we will use the “PackageName”.
The Product Code will be populated with the "PackageIdentifier" if it is not specified in the “AppsAndFeaturesEntry”.

No shortcuts or icons will be created for the package.

>Note: Some portable programs may require dependencies. Dependencies are not covered in this specification and are not yet supported as of the writing of this specification.

### Upgrade

The package is upgraded in the same path as the installed version. The first step the Windows Package Manager will perform is an uninstall of the previous version of the package. If the file is in use, the user will be informed the package is running so they can shut it down. Optionally, the user may specify "--force" to forcefully shut the application down for upgrade.

A new Windows Apps & Features entry is created to correctly report the upgraded version for future potential upgrades.

There will be no support for installing multiple “side by side” versions of portable packages.

### Uninstall

The executable should be removed along with the entry in Apps & Features. The user should also be able to uninstall the package from Apps & Features.

>Note: the default behavior for uninstalling a portable application with Windows Apps & Features will be to execute uninstall with "--purge" so any files created by the portable application will be removed if they are located in the portable applications directory.

If the directory is empty after removing the portable application, the directory should also be deleted. If the directory is not empty, the user should be informed that other files exist in the directory so it will not be removed.

### Manifest Validation

Portable package manifests will only support zero or one "command" value to be specified. In the absence of the "--rename" argument, the value specified in "command" will define the default value for renaming the portable application and subsequently the value specified in the "AppPath" subkey.

## UI/UX Design

### Installing a portable package

```text
winget install Microsoft.NuGet
Found NuGet [Microsoft.NuGet] Version 6.0
This application is licensed to you by its owner.
Microsoft is not responsible for, nor does it grant any licenses to, third-party packages.
Downloading https://dist.nuget.org/win-x86-commandline/v6.0.0/nuget.exe
  ██████████████████████████████   0.1 MB /  0.1 MB
Successfully verified installer hash
Starting package install...
Successfully installed
```

### Upgrading a portable package

```text
winget upgrade Microsoft.NuGet
Found NuGet [Microsoft.NuGet] Version 7.0
This application is licensed to you by its owner.
Microsoft is not responsible for, nor does it grant any licenses to, third-party packages.
Downloading https://dist.nuget.org/win-x86-commandline/v7.0.0/nuget.exe
  ██████████████████████████████   0.1 MB /  0.1 MB
Successfully verified installer hash
Starting package install...
Successfully installed
```

If the portable application is running when the user performs an upgrade, the user will be informed and given options.

```text
winget upgrade Microsoft.NuGet
Found NuGet [Microsoft.NuGet] Version 7.0
This application is licensed to you by its owner.
Microsoft is not responsible for, nor does it grant any licenses to, third-party packages.
Downloading https://dist.nuget.org/win-x86-commandline/v7.0.0/nuget.exe
  ██████████████████████████████   0.1 MB /  0.1 MB
Successfully verified installer hash
Starting package install...
Failed to install NuGet [Microsoft.Nuget].
Package in use. Either exit the program or use "--force" to upgrade.
```

### Uninstalling a portable package

```text
winget uninstall NuGet 
Found NuGet [Microsoft.NuGet]
Starting package uninstall...
Successfully uninstalled
```

If the portable application created files in the portable application's directory, the user will be informed.

```text
winget uninstall NuGet 
Found NuGet [Microsoft.NuGet]
Starting package uninstall...
The "--purge" argument was not specified. 
Files still exist in "%LOCALAPPDATA%/WinGet_Packages/User/Microsoft.NuGet/". 
Successfully uninstalled
```

## Capabilities

This will allow users to manage portable applications via the Windows Package Manager.

### Accessibility

Any description or output text that is added by consequence of this feature will need to be localized. Moreover, this feature allows a mechanism to add accessibility settings in the future.

### Security

This feature will not add any new security risks. All risks associated with this feature have been resolved with the core install, import, upgrade, and uninstall features.

>Note: We recommend users leverage the Windows Sandbox when testing manifests on their local machines as the programs may not have been validated from a security perspective prior to being submitted to the Windows Package Manager Community App Repository.

### Reliability

This should not introduce any new reliability concerns.

### Compatibility

Previous versions of the Windows Package Manager will not cease to function, but they will not be able to apply arguments or settings related to this feature. They will not be able to install portable applications.

An update to the Windows Package Manager is required to use this feature with the related arguments and settings.

### Performance, Power, and Efficiency

This should not introduce any new performance, power, or efficiency concerns in the Windows Package Manager.

## Potential Issues

Users may not be aware that there is no support for multiple versions of the same package on their system, so performing an upgrade may not provide they experience they were expecting.

Users may not be aware that the uninstall behavior by default may leave files on their systems if they were created by a portable application.

If a user performs uninstall via Windows Apps & Features the default behavior will be to run `winget uninstall package --purge`.

## Future considerations

## Resources

[Portable / Standalone Executables · Discussion #44183 · microsoft/winget-pkgs (github.com)](https://github.com/microsoft/winget-pkgs/discussions/44183)

[Application Registration - Win32 Apps](https://docs.microsoft.com/en-us/windows/win32/shell/app-registration)

