---
author: Alexis Bekhdadi @midoriiro
created on: 2021-11-16
last updated: 2021-11-17
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

Regarding case 1, installation flow check if TEMP folder contain a single binary and repack that binary
into an MSI(X) package (including setting PATH env var). Then, MSI(X) installation flow can take control.

Regarding case 2, installation flow check if TEMP folder contain an installer (.exe, .msi, .msix) and pass control
to MSI(X) installation flow.

Regarding case 3, installation flow check if TEMP folder does not contain an installer nor a single binary.
This case is the complex one, example with 'vagrant' hierarchy:
- root
  - bin 
    - vagrant.exe
    - other.exe
  - embedded
    - bin
    - some other folders

This hierarchy can be packed into an MSI(X) installer, but how can we specify which binary to add 
in PATH env var (in that case bin/vagrant.exe)?

This specification take on consideration schema on version 1.1.0. 
A schema update should be necessary to take all the cases in consideration. 

Take a look into the following manifest examples

### Case 1|2 YAML manifest

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

### Case 2.1 YAML manifest

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
ManifestType: installer
ManifestVersion: 1.0.0
```

This is not compliant with the actual schema v1.1.0.
```InstallerPath``` attribute should be added to schema.

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

### Repack uncompressed files into a supported package format

There are many tool to repack an MSI(X) package, some expansive and some apparently free (Advanced Installer ?).
However, Windows SDK provide a CLI tool called MakeAppx in order to create MSIX package.

## UI/UX Design

One downside of this feature is the package will be built on the client side.
if the uncompressed files need to be packaged into an MSI(X) installer, that mean installing on client side
an external or internal tool to repacking the uncompressed files is needed. In the case of MakeAppx, that mean
installing a Windows SDK.

But with a proper MSI(X) repacking solution, this operation should be transparent to the end-user.
- For case 2, the existing installation flow will take control and behave as expected
- For case 1 and 3, the new installation flow need to behave like other installation flow (showing steps and progress) 

## Capabilities

### Accessibility

This should have no direct impact on accessibility.

### Security

Need more specification

### Reliability

This is not expected to impact reliability.

### Compatibility

This feature is intended be an experimental one, at least for a first step.
End-user need to toggle on the feature in order to install zip packages. 

No code breaking, reuse existing code and add code to support zip installation.

### Performance, Power, and Efficiency

Using an internal/external repacking MSI(X) installer solution will impact client device performance. 

## Potential Issues

Need more specification

## Future considerations

Initial implementation will only take in consideration .zip archive. However, when the feature will be stable
we can consider other types of archives (e.g. *.gz. *.7z, etc). 

## Resources
[MakeAppx.exe Documentation](https://docs.microsoft.com/en-us/windows/msix/package/manual-packaging-root)