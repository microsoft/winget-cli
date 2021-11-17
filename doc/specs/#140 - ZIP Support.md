---
author: Alexis Bekhdadi @midoriiro
created on: 2021-11-16
last updated: 2021-11-16
issue id: 140
---

# Spec Title

For [#140](https://github.com/microsoft/winget-cli/issues/140)

## Abstract

This feature adds to winget-cli the ability to install application deserved thought a ZIP archive.

## Inspiration

A bunch of applications are only provided by developer's through a ZIP archive. There are three cases:
1) The archive contains only one binary (a portable / standalone executable).
2) The archive contains only one supported installer (msix, msi, or exe).
3) The archive contains more than one file.

## Solution Design

A new installation flow need to be created to execute different installation strategies regarding the three cases above.

First thing first, the archive needs to be extracted in a specific path (TEMP folder can be a good candidate).

Regarding case 1, installation flow check if TEMP folder contain a single binary and repack that binary
into an MSI package (including setting PATH env var). Then, MSI installation flow can take control.

Regarding case 2, installation flow check if TEMP folder contain an installer (.exe, .msi, .msix) and pass control
to MSI installation flow.

Regarding case 3, installation flow check if TEMP folder does not contain an installer nor a single binary.
This case is the complex one, example with 'vagrant' hierarchy:
- root
  - bin 
    - vagrant.exe
    - other.exe
  - embedded
    - bin
    - some other folders

This hierarchy can be packed into an MSI installer, but how can we specify which binary to add 
in PATH env var (in that case bin/vagrant.exe)?

Take a look into the following manifest examples (v1.1.0)

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
  BinaryPath: bin/binary.exe
ManifestType: installer
ManifestVersion: 1.0.0
```

This is not compliant with the actual schema v1.1.0. We need to specify which binary to add in
PATH env var. Therefore, schema need to be updated to take knowledge of that information.

### Repack ZIP into MSI package

This section need more specifications. There are many tool to repack an MSI package, some expansive and
some apparently free (Advanced Installer ?).

## UI/UX Design

One downside of this feature is the MSI repackaging will be executed on client side.
Which mean for each installation the same process will be executed and remain the same for an 
application x.y.z.

But with a proper MSI repacking solution, this operation should be transparent to the end-user.

## Capabilities

### Accessibility

This should have no direct impact on accessibility.

### Security

No direct impact on security.

### Reliability

This is not expected to impact reliability.

### Compatibility

No code breaking, reuse existing code and add code to support zip installation.

### Performance, Power, and Efficiency

## Potential Issues

## Future considerations

## Resources