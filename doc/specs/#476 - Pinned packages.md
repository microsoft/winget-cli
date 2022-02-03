---
author: Easton Pillay jedieaston/easton@planeteaston.com
created on: 2022-01-25
last updated: 2022-02-02
issue id: 476
---

# Pinning a package

For [#476](https://github.com/microsoft/winget-cli/issues/476)

## Abstract

This spec describes the functionality behind "pinning" a package (freezing a package at a certain version so that it will be not be automatically upgraded via `winget upgrade --all`).

## Inspiration

This functionality is inspired by functionality in other package managers, as well as community feedback. Many packages (software development tools and libraries commonly, but other software as well) introduce breaking changes that users may not want integrate into their workflow quite yet.

## Solution Design

A table of packages (by Name, Version and Product Code if available) that are currently pinned will be kept in the local tracking catalog. To be repository independent, these values should be correlated from local sources (Add and Remove Programs, Appx, etc). This also means that the user should be able to pin packages that were not installed via the Windows Package Manager, so that they won't be upgraded via `winget upgrade --all`. 


## UI/UX Design

Several commands will need to be added to support pinning packages. Where applicable, equivalent names (or new arguments) for PowerShell cmdlets will also be given.

The main interface for the pinning feature will be a new command, `winget pin`, with a couple of options:


```
winget pin
PowerShell: Get-WinGetPinnedPackage
```

Running `winget pin` with no arguments will show a table of currently pinned packages:

```
Name                                                 Id                                Version    
------------------------------------------------------------------------------------------------
Microsoft Bob                                        Microsoft.Bob                     2.35.0
iTunes                                               Apple.iTunes                      12.0.199.4

```

```
winget pin --set <package>
PowerShell: Set-WinGetPinnedPackage
```

`<package>` correlates to a query, similar to `winget list`. When running this command, the package information will be stored in the table in the tracking catalog. This command should be able to accept Package Identifers (checked against all repositories) as well as names, tags, etc. If the package does not have a version number locally but is available from a source, then the version number from the source should be used.

If there is no package directly available for software a user wants to pin, but it is [included as part of another package](https://github.com/microsoft/winget-cli/issues/1073), the package which includes said piece of software should be treated as pinned unless a newer version doesn't change the version of the pinned software. 

```
winget pin --clear <package>
PowerShell: Remove-WinGetPinnedPackage
```

It functions similarly to `winget pin --set`, but it clears the pin. This should automatically happen on `winget uninstall`, or if it is detected the package is no longer present on the system.

Enhancements will also need to be made to other commands in the Windows Package Manager, including:

```
winget upgrade --include-pinned
PowerShell: Get-WinGetPackage -UpgradeAvailable -Pinned
```

With the new `--include-pinned` argument, Upgrade should show a separate table in addition to the regular table which shows possible upgrades to pinned packages.

```
<...>
Upgrades to pinned packages: 

Name                                                 Id                                Version       Available     Source
-------------------------------------------------------------------------------------------------------------------------
Clippy for Azure 2011 Datacenter                     Microsoft.Clippy.2011.Datacenter  2.35.0        2.35.1.2      winget
```


```
winget <upgrade/install> --include-pinned <package>
PowerShell: Install-WinGetPackage -Pinned / Upgrade-WinGetPackage -Pinned
```

Upgrade and Install will now need to use the same argument to bypass the pin for a package temporarily, in which case the pin is updated to pin the new version after the installation completes.

```
winget install --pin <package>
```

Install should also be extended to allow the user to pin a package at install time.



In addition to commands, there should be a new `Pinned` key in exported package lists, allowing users to move their pins to different systems:

```
// ...
    {
      "Packages": 
      [
        {
          "PackageIdentifier": "Microsoft.Bob",
          "PackageVersion" : 2.35.0,
          "Pinned" : true
        },
      ],
    }
// ...
```

Lastly, there should be good error messaging throughout the Windows Package Manager in order to explain to users when something doesn't go quite right. 

As an example, consider the situation where a user has pinned their favorite package, Microsoft Bob:

```
PS C:\Users\billg> winget install --pin Microsoft.Bob
Found Microsoft Bob [Microsoft.Bob] Version 2.32.0
This application is licensed to you by its owner.
Microsoft is not responsible for, nor does it grant any licenses to, third-party packages.
Downloading https://aka.ms/importantsystemdependencies/bob.exe
██████████████████████████████ 2.08 MB / 2.08 MB
Successfully verified installer hash
Starting package install...
Successfully installed
Microsoft.Bob has been pinned at version 2.32.0.
```

Then, later, that user tries to install a package that is dependent on a newer version of Microsoft Bob. The Windows Package Manager should not upgrade Microsoft Bob automatically, but instead should notify the user and give them information on how to continue:

```
PS C:\Users\billg> winget install --pin Microsoft.RoverTheDog
Found Rover the Dog [Microsoft.RoverTheDog] Version 5.1
This application is licensed to you by its owner.
Microsoft is not responsible for, nor does it grant any licenses to, third-party packages.
This package requires the following dependencies:
  - Packages
      Microsoft.Bob, version 2.39.0.

Microsoft Bob is currently pinned at version 2.32.0, but Rover the Dog 5.1 requires at least version 2.39.0. 
To upgrade Microsoft Bob and other pinned dependencies, add the argument --include-pinned to your previous command.
```

## Capabilities

### Accessibility

Accessibility should not be impacted by this change. There will be a few more tables printed to the terminal in certain cases, but they should use the current table implementation used by `winget upgrade` and `winget list`.

### Security

Security of the Windows Package Manager should not be impacted by this change. However, security of user's software may be, as if they pin a insecure version of a package they currently will not be notified of important security updates within the Windows Package Manager.

### Reliability

The change will improve reliability, as users will be able to have fine grained control of the Windows Package Manager's upgrade functionality to ensure their workflow is not disrupted. 

### Compatibility

There should not be any breaking changes to the code. Although there could be a mild breaking change to the behavior of `upgrade --all` (not all packages are upgraded anymore since pinned ones are skipped), this is purely opt-in from the user's perspective at this time (if they do not pin software, there should not be a change).

### Performance, Power, and Efficiency

There should not be any notable performance changes.


## Potential Issues

Outside of missing security updates and having possible dependency resolution failures due to pins, there should not be any other impacts. Considering the opt-in nature of pinning, it should not affect users unless they want to use it.

## Future considerations

- Group Policy or MDM control of pinned packages would be a natural addition to the defined functionality. Administrators may want to restrict upgrades for certain pieces of conflicting software if it will cause issues for some or all of their users.

- Packages may want to define pins for their dependencies, if they can't use anything above a certain version of a dependency. These pins could be merged into the user's current pins, making sure that the packages currently installed keep working.

## Resources

- [Brew - How do I stop certain formulae from being upgraded?](https://docs.brew.sh/FAQ#how-do-i-stop-certain-formulae-from-being-updated)

- [NPM - package.json dependencies](https://docs.npmjs.com/cli/v7/configuring-npm/package-json#dependencies)

- [APT - Introduction to Holding Packages](https://help.ubuntu.com/community/PinningHowto#Introduction_to_Holding_Packages)

