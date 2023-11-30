---
author: Demitrius Nelon @denelon
created on: 2023-11-30
last updated: 2023-11-30
issue id: 2129
---

# Better support for Side by Side packages

[comment]: # Link to issue: "For [#2929](https://github.com/microsoft/winget-cli/issues/2129)"

## Abstract

WinGet should better support multiple versions of the same package being installed on the same machine. This happens, and WinGet currently tries to pretend it doesn't.

## Inspiration

Multiple [issues](https://github.com/microsoft/winget-cli/issues?q=is%3Aissue+is%3Aopen+label%3Aside-by-side) have been submitted with the "side-by-side" label.

## Solution Design

Per comment by @JohnMcPMS:
> Replace PackageVersion InstalledVersion with PackageVersion[] InstalledVersions internally, then reasoning about how that affects everywhere that breaks.

## UI/UX Design

Several different commands will be impacted by this change. In some cases, additional information will be presented to the user so they are aware of the implications for the command executed.

### WinGet Install

When one or more versions (but not the latest) of a package are installed on the system and none of them are pinned, running `winget install <package>` will install the latest version of the package side-by-side unless the manifest specifies the previous version should be uninstalled or denied. The user may override this behavior with "--retain-previous".
> [!NOTE]  
> WinGet will not attempt to override the default behavior the installer may have related to the removal of previous versions of the package.

### WinGet List

When more than one version of a package is installed on the system, all installed versions will be listed. Only the latest version of an installed package will show results in the "Available" column if a newer version is available in a configured source.

### WinGet Upgrade

When more than one version of a package is installed on the system, only the latest installed version will be listed if a newer version is available in a configured source. If the latest available version is in the collection of installed packages, no upgrade will be listed.

### WinGet Uninstall

For `winget uninstall <package>`:

When more than one version of a package is installed on the system, the user will be prompted to disambiguate. The user will be informed "Multiple package versions found matching input criteria. Please specify the version to be uninstalled or use "--all" to remove all versions of the package". All installed versions will be displayed so the user may either disambiguate by specifying the version to be removed or by passing "--all" as an argument to specify every installed version of the package should be uninstalled.

For `winget uninstall <package> --version <version>`:

If the version specified is one of the installed versions, only that version will be uninstalled. 

### WinGet Pin

The current behaviors associated with pinned packages will continue to be honored.

## Capabilities

Many different commands will be impacted as WinGet will understand the notion of multiple versions of the same package may be installed. This will necessarily be complicated when one or more versions of a package are pinned, and the pins should be prioritized in terms of behavior.

### Accessibility

There should be no noteworthy impacts to accessibility.

### Security

There should be no noteworthy impacts to security.

### Reliability

There should be no noteworthy impacts to reliability.

### Compatibility

There should be no noteworthy impacts to compatibility.

### Performance, Power, and Efficiency

## Potential Issues

Some package installers may remove one or more previous versions of packages upon install. WinGet will not be able to override these behaviors, so in some cases, users may not be pleased with the results.

Some packages are isolated into unique package identifiers based on major or minor versioning. When WinGet is attempting to correlate an installed version of a package during an upgrade scenario this may result in matching with more than one package in a configured source. This is often the case with runtimes or software development kits. 

For example, consider the following packages are available.

SDK.Runtime.1
SDK.Runtime.2

Assume SDK.Runtime.1 has versions 1.0, 1.1, and 1.2 available.
Assume SDK.Runtime.2 has versions 2.0, and 2.1 available.

The user may have SDK.Runtime.1 versions 1.0 and 1.1 installed as well as SDK.Runtime.2 version 2.0 installed. 

If sufficient metadata isn't provided for WinGet to correlate both logical packages have updates available, the user may only be provided with the SDK.Runtime.2 version 2.1 as an available upgrade.

## Future considerations

Some packages are isolated into unique package identifiers based on major or minor versioning. In the future, WinGet may have some form or package providers or virtual packages to simplify getting or installing the latest version. 

## Resources
