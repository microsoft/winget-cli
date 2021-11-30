---
author: Alexis Bekhdadi @midoriiro
created on: 2021-11-16
last updated: 2021-11-30
issue id: 140
---

# Zip Support

For [#140](https://github.com/microsoft/winget-cli/issues/140)

## Abstract

This feature adds to winget-cli the ability to install application delivered through a ZIP archive.

## Inspiration

A bunch of applications are only provided by developer's through a ZIP archive. There are three cases:
1) The archive contains only one binary (a portable / standalone executable).
2) The archive contains only one supported installer (msix, msi, or exe).
3) The archive contains more than one file.

## Solution Design

A new installation flow need to be created to execute different installation strategies regarding the three cases above.

First thing first, the archive needs to be extracted in a specific path (TEMP folder can be a good candidate).

Regarding case 1, installation flow check if TEMP folder contain a single binary and pass instruction to 
installation flow (including setting PATH env var).

Regarding case 2, installation flow check if TEMP folder contain an installer (.exe, .msi, .msix, etc) and pass control
to proper installation flow.

Regarding case 3, installation flow check if TEMP folder does not contain an installer nor a single binary.
This case is the complex one, example with 'vagrant' hierarchy:
- root
  - bin 
    - vagrant.exe
    - other.exe
  - embedded
    - bin
    - some other folders

This hierarchy can be packed into an MSI(X) installer or register the application in ARP at the end of installation 
flow (see section below about repack or ARP entry).

This specification take on consideration schema on version 1.1.0. 
A schema update should be necessary to take all the cases in consideration. 

Take a look into the following manifest examples

### Case 1 YAML manifest

```
PackageIdentifier: Editor.App
PackageVersion: 1.0
MinimumOSVersion: 10.0.0.0
InstallerType: zip
Scope: ...
InstallModes: ...
InstallerSwitches:
  Silent: ...
  SilentWithProgress: ...
UpgradeBehavior: install
Installers:
- Architecture: x64
  InstallerUrl: https://editor.com/app/1.0.zip
  InstallerSha256: ...
ManifestType: installer
ManifestVersion: 1.0.0
```

This is compliant with the actual schema v1.1.0

### Case 2 YAML manifest

If the archive contains an installer but not located at root folder (e.g. archive.zip/somefolder/installer.exe)

```
PackageIdentifier: Editor.App
PackageVersion: 1.0
MinimumOSVersion: 10.0.0.0
InstallerType: zip
Scope: ...
InstallModes: ...
InstallerSwitches:
  Silent: ...
  SilentWithProgress: ...
UpgradeBehavior: install
Installers:
- Architecture: x64
  InstallerUrl: https://editor.com/app/1.0.zip
  InstallerSha256: ...
  InstallerPath: somefolder/installer.exe
  NestedInstallerType: msi
ManifestType: installer
ManifestVersion: 1.0.0
```

This is not compliant with the actual schema v1.1.0.
```InstallerPath``` key should be added to schema. 
This optional key define where to find installer file after extraction if not located at root.

To specify installer type, ```NestedInstallerType``` key must be defined.
As mentioned in issue [#1242](https://github.com/microsoft/winget-cli/issues/1242).

### Case 3 YAML manifest

```
PackageIdentifier: Editor.App
PackageVersion: 1.0
MinimumOSVersion: 10.0.0.0
InstallerType: zip
Scope: ...
InstallModes: ...
InstallerSwitches:
  Silent: ...
  SilentWithProgress: ...
UpgradeBehavior: install
Installers:
- Architecture: x64
  InstallerUrl: https://editor.com/app/1.0.zip
  InstallerSha256: ...
  BinaryPath: bin (e.g. bin folder where vagrant.exe is located in my example)
ManifestType: installer
ManifestVersion: 1.0.0
```

This is not compliant with the actual schema v1.1.0. 
We need to specify which binary folder to add in PATH env var.
```BinaryPath``` attribute should be added to schema.

### Processing uncompressed files after extraction

Two solutions can be taken in consideration. Repack uncompressed files into an installer or add an ARP entry in
registry.

For case 2, the manifest should contain all information needed to other installation flow. There is no need to repack 
or add an ARP entry.

For all others cases, repacking a file structure might not be a good solution. This process require installing on client
side a third party repacking solution or installing Windows SDK that provide MakeAppx that repack our file structure
into a MSIX installer. But not everyone needs to install Windows SDK and repacking can take time and resources.

Adding an ARP entry into registry can be done more easily, actually winget-cli already read ARP entries through 
[ARPHelper](https://github.com/microsoft/winget-cli/blob/master/src/AppInstallerRepositoryCore/Microsoft/ARPHelper.cpp) 
struct. 

Windows [documentation](https://docs.microsoft.com/en-us/windows/win32/msi/uninstall-registry-key) define properties for
an ARP entry (these properties are also defined in ARPHelper struct).

An ARP entry can be defined in two location:
 - HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\Uninstall (scope machine)
 - HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall (scope user)

Uninstall key contains sub keys identified by the application product code, but for our case 1 and 3 a product code
might not exist. Two solutions can be considered, the package maintainer can generate a random GUID but this identifier 
should be reused on each manifest for a particular application. The second solution is to set the sub key name by the 
application name which can be retrieved on the manifest with the ```PackageName``` key.

Then the installation flow should register an ARP entry with the proper properties defined in manifest.

Our file structure should be moved into a proper installation folder.
Depending on the scope and the architecture we have different folders:
  - %HOMEDRIVE%%HOMEPATH%/AppData/Local (scope user)
  - %PROGRAMFILES% (scope machine, architecture x64)
  - %PROGRAMFILES(X86)% (scope machine, architecture x86)

> Above section might need more specification about where to install the file structure

Finally, if manifest key ```BinaryPath``` is defined the installation flow should add to PATH environment variable the
resolved binary path folder.

## UI/UX Design

If we exclude repacking solutions and prefer add/edit ARP entry, that should be transparent to the end-user.

- For case 2, the existing installation flow will take control and behave as expected
- For case 1 and 3, the new installation flow need to behave like other installation flow (showing steps and progress) 

## Capabilities

### Accessibility

This should have no direct impact on accessibility.

### Security

> Need more specification

### Reliability

This is not expected to impact reliability.

### Compatibility

This feature is intended be an experimental one, at least for a first step.
End-user need to toggle on the feature in order to install zip packages. 

No code breaking, reuse existing code and add code to support zip installation.

### Performance, Power, and Efficiency

Add/edit an ARP entry should not be an issue for performance, power and efficiency 

## Potential Issues

> Need more specification

## Future considerations

Initial implementation will only take in consideration .zip archive. However, when the feature will be stable enough,
we can consider other types of archives (e.g. *.gz. *.7z, etc). 

## Resources
[MakeAppx.exe Documentation](https://docs.microsoft.com/en-us/windows/msix/package/manual-packaging-root)