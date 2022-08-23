---
author: Demitrius Nelon @denelon
created on: 2022-03-09
last updated: 2022-04-07
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

Some packages like GitLab Runner have a file name including extra metadata (gitlab-runner-windows-386.exe). The installation instructions suggest renaming the file after it has been downloaded. In that vein, a "--rename" argument should be added so the user can choose a value they prefer. To have the upgrade scenario honor the custom name, this information should be recorded in the installed packages data store.

Portable / standalone executables should have their “command” value specified so the Windows Package Manager can determine the default value to use when creating a new entry in the "App Paths" registry during installation. If no command value is provided, then the entry will use the filename of the exe. 

Some portable applications generate or consume other files. An additional argument "--purge" will be added for the uninstall scenario to remove all files and subsequently the directory if the user wishes. The corollary argument "--preserve" will be used to preserve files if the default setting has been modified.

### Settings

Users should have settings to be able to specify a new default location other than the system default for the Windows Package Manager.

The default path for installing these packages is "%LOCALAPPDATA%/Microsoft/WinGet/Packages/" for user based installs. The default path for installing these packages is "Program Files/WinGet/Packages/" for machine wide x64 installs and "Program Files (x86)/WinGet/Packages/" for machine wide x86 installs.

The corresponding settings are "PortablePackageUserRoot" and "PortablePackageMachineRoot".

>Note: The "packageIdentifier" will be used to generate subdirectories for these binaries to be installed to.

Using Gitlab.gitlab runner as an example would result in the .exe being placed in "%LOCALAPPDATA%/Microsoft/WinGet/Packages/Gitlab.gitlab-runner/"
If a user has configured a path to be used for portable applications, that path should be honored. The user should also be able to specify if they want all portable packages placed in the same directory, or in a directory per package.

A related setting for the uninstall scenario will be able to specify the default behavior for either "--clean" or "--purge".

### Install

When the portable application is being installed by the Windows Package Manager, an entry will be created in Windows Apps & Features so the user will be able to see that the application is installed. 

Once the portable application is copied to the appropriate install location based on the preferences of the user, a symlink will be created that points to the portable application. The locations where these symlinks will be stored are "%LOCALAPPDATA%/Microsoft/WinGet/Links/" for user based installs, "Program Files/WinGet/Links/" for machine wide x64 installs and "Program Files (x86)/WinGet/Links/" for machine wide x86 installs. We will then append these paths to the PATH environment variable if they do not exist already.

The filename of the symlink file determines the command alias that will be used when being executed in the command prompt. By default, the symlink filename we be the same as the filename of the portable exe. If a command value is specified in the manifest, then the symlink filename will be renamed to match the command value. If the rename argument is provided, then both the symlink filename and the portable exe filename will be renamed to match the rename argument value.

>Note: In our initial design, we had opted to only writing to the "AppPath" registry subkey to install/register the portable application. However, we determined that this is not sufficient to support command-line execution, which does not look in the "AppPath" registry when locating executables.

The data from “AppsAndFeaturesEntry” in the manifest will be used to specify the name of the package in Windows Apps & Features.  Constraints for manifests with portable packages are covered below in the section on validating manifests. If the “AppsAndFeaturesEntry” doesn’t have a “DisplayName” value, then we will use the “PackageName”.
The Product Code will be populated with the "PackageIdentifier" if it is not specified in the “AppsAndFeaturesEntry”.

No shortcuts or icons will be created for the package.

>Note: Some portable programs may require dependencies. Dependencies are not covered in this specification and are not yet supported as of the writing of this specification.

#### Scope Behavior

By default, portable apps will be installed with the "User" scope unless specified otherwise by the user through the "--scope" argument or in their settings. This also means that the "scope" field in the manifest will be ignored when installing portable apps. Since portable apps are unique in that they are simply standalone executables that are copied to a specific install location based on scope, it does not seem reasonable to have the manifest enforce an install behavior that is not required. Manifest validation would also need to be updated to show an error to the user that the "scope" field does not apply if a portable installer type is specified.

#### Installation from multiple sources:

If the user chooses to install the same package but from a secondary source, the Windows Package Manager will append the source name to subdirectory. For example, if GitLabRunner is installed a second time but from the msstore, then the full path would be "%LOCALAPPDATA%/Microsoft/WinGet/Packages/Gitlab.GitLabRunner_msstore/". 

The same behavior will be applied when creating a symlink in order to avoid overwriting an existing symlink of the same package but from a different source. Using the same example, the generated symlink for GitLabRunner from the msstore will have a full path of "%LOCALAPPDATA%/Microsoft/WinGet/Links/Gitlab.GitLabRunner_msstore.exe/"

### Upgrade

The package is upgraded in the same path as the installed version. The first step the Windows Package Manager will perform is to download the executable to a temporary location, and attempt to copy the exe to the specified install location. If an exe with the same name already exists, the Windows Package Manager will attempt to overwrite the file. If that process fails because the file is currently in use, the user will be informed the package is running so they can shut it down. Optionally, the user may specify "--force" to forcefully shut the application down for upgrade. Once the exe has successfully been copied to the specified install location, the entry in "Apps & Features" will be updated accordingly and the symlink will be overwritten to point to the latest portable exe.

If the "UninstallPrevious" field is specified in the manifest, then the Windows Package Manager will perform an uninstall of the previous version of the package prior to installing the newer version. 

A new Windows Apps & Features entry is created to correctly report the upgraded version for future potential upgrades.

There will be no support for installing multiple “side by side” versions of portable packages.

### Uninstall

The executable and the symlink should be removed along with the entry in Apps & Features. The user should also be able to uninstall the package from Apps & Features.

>Note: The default behavior for uninstalling a portable application with Windows Apps & Features will be to execute uninstall without "--purge" so any files created by the portable application will be remain if they are located in the portable applications directory. An additional argument for "--wait" will be added that will prompt the user to press any key to exit. This is intended to support this scenario so that the user is aware of any remaining files if they choose to uninstall through Windows Apps & Features.

If the directory is empty after removing the portable application, the directory should also be deleted. If the directory is not empty, the user should be informed that other files exist in the directory so it will not be removed. 

### Manifest Validation

Portable package manifests will only support zero or one "command" value to be specified. In the absence of the "--rename" argument, the value specified in "command" will define the default value for naming the symlink file that is created to point to the portable application.

If a value for "scope" is specified for a portable installer, the user should be shown an error that the "scope" field is not supported for portable installers.

Only zero or one entry for "Apps & Features" can be specified for a portable installer.

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

If the portable application is running when the user performs an upgrade, the user will be informed.

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

If a user performs uninstall via Windows Apps & Features the default behavior will be to run `winget uninstall package --purge`. If a user removes the "App Installer" they would not be able to perform uninstall.

Displaying a path may be improved with detecting the shell the Windows Package Manager is being executed in. An [Issue](https://github.com/microsoft/winget-cli/issues/1977) was already raised covering the differences in how CMD and PowerShell render or support file system paths.

## Future considerations

Some portable applications have icons. The options for creating shortcuts with icons could be considered in the future.

Support for channels has not yet been implemented. Some portable applications may have "beta" and "stable" releases. Users may want to have support for both in the future.

## Resources

[Portable / Standalone Executables · Discussion #44183 · microsoft/winget-pkgs (github.com)](https://github.com/microsoft/winget-pkgs/discussions/44183)

[Application Registration - Win32 Apps](https://docs.microsoft.com/en-us/windows/win32/shell/app-registration)

