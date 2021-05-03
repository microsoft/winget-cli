---
title: Create your package manifest
description: If you want to submit a software package to the Windows Package Manager repository, start by creating a package manifest.
ms.date: 04/29/2020
ms.topic: article
ms.localizationpriority: medium
---

# Create your package manifest

[!INCLUDE [preview-note](../../includes/package-manager-preview.md)]

If you want to submit a software package to the [Windows Package Manager repository](repository.md), start by creating a package manifest. The manifest is a YAML file that describes the application to be installed.

This article describes the contents of a package manifest for Windows Package Manager.

## YAML basics

The YAML format was chosen for package manifests because of its relative ease of human readability and consistency with other Microsoft development tools. If you are not familiar with YAML syntax, you can learn the basics at [Learn YAML in Y Minutes](https://learnxinyminutes.com/docs/yaml/).

> [!NOTE]
> Manifests for Windows Package Manager currently do not support all YAML features. Unsupported YAML features include anchors, complex keys, and sets.

## Conventions

These conventions are used in this article:

* To the left of `:` is a literal keyword used in manifest definitions.
* To the right of `:` is a data type. The data type can be a primitive type like **string** or a reference to a rich structure defined elsewhere in this article.
* The notation `[` *datatype* `]` indicates an array of the mentioned data type. For example, `[ string ]` is an array of strings.
* The notation `{` *datatype* `:` *datatype* `}` indicates a mapping of one data type to another. For example, `{ string: string }` is a mapping of strings to strings.

## Manifest contents

A package manifest must include a set of required items, and can also include further optional items that can help improve the customer experience of installing your software. This section provides brief summaries of the required manifest schema and complete manifest schemas, and examples of each.

Each field in the manifest file must be Pascal-cased and cannot be duplicated.

For a complete list and descriptions of items in a manifest, see the [manifest specification](https://github.com/microsoft/winget-cli/blob/master/doc/ManifestSpecv0.1.md) in the [https://github.com/microsoft/winget-cli](https://github.com/microsoft/winget-cli) repository.

### Minimal required schema

#### [Minimal required schema](#tab/minschema/)

```yaml
Id: string # Publisher.package format.
Publisher: string # The name of the publisher.
Name: string # The name of the application.
Version: string # Version numbering format.
License: string # The open source license or copyright.
InstallerType: string # Enumeration of supported installer types (exe, msi, msix, inno, wix, nullsoft, appx).
Installers:
  - Arch: string # Enumeration of supported architectures.
    Url: string # Path to download installation file.
    Sha256: string # SHA256 calculated from installer.
ManifestVersion: 0.1.0
```

#### [Example](#tab/minexample/)

```yaml
Id: microsoft.teams
Publisher: Microsoft Corporation
Name: Microsoft Teams
Version: 1.3.0.4461
License: Copyright (c) Microsoft Corporation. All rights reserved.
InstallerType: exe
Installers:
  - Arch: x64
    Url: https://statics.teams.cdn.office.net/production-windows-x64/1.3.00.4461/Teams_windows_x64.exe
    Sha256: 712f139d71e56bfb306e4a7b739b0e1109abb662dfa164192a5cfd6adb24a4e1
ManifestVersion: 0.1.0
```

* * *

### Complete schema

#### [Complete schema](#tab/compschema/)

```yaml
Id: string # Publisher.package format.
Publisher: string # The name of the publisher.
Name: string # The name of the application.
AppMoniker: string # The common name someone may use to search for the package.
Version: string # Version numbering format for package version.
Channel: string # A string representing the flight ring.
License: string # The open source license or copyright.
LicenseUrl: string # Valid secure URL to license.
MinOSVersion: string # Version numbering format for minimum version of Windows supported.
Description: string # Description of the package.
Homepage: string # Valid secure URL for the package.
Tags: list # Additional strings a user would use to search for the package.
FileExtensions: list # List of file extensions the package could support.
Protocols: list # List of protocols the package provides a handler for.
Commands: list # List of commands or aliases the user would use to run the package.
InstallerType: string # Enumeration of supported installer types (exe, msi, msix, inno, wix, nullsoft, appx).
Switches: # These can be used to change the install behavior if supported by the InstallerType.
  Custom: string # Custom switches passed to the installer.
  Silent: string # Switches passed to the installer for silent installation.
  SilentWithProgress: string # Switches passed to the installer for non-interactive install.
  Interactive: string # Experimental.
  Language: string # Experimental.
  Log: string # Specifies log redirection switches and path.
  InstallLocation: string # Specifies alternate location to install package.
Installers: # Nested map of keys for specific installer.
  - Arch: string # Enumeration of supported architectures.
    Url: string # Path to download installation file.
    Sha256: string # SHA256 calculated from installer.
    SignatureSha256: string # SHA256 calculated from signature file's hash of MSIX file.
    Switches: # Collection of entries to override root keys. The primary supported values are: Custom, Silent, SilentWithProgress, Interactive. For a complete list see the specification at https://github.com/microsoft/winget-cli/blob/master/doc/ManifestSpecv0.1.md.
    Scope: string # Experimental.
    SystemAppId: string # Experimental.
Localization: # Nested map of keys for localization.
  - Language: string # Locale for display fields and localized URLs.
ManifestVersion: string # Version number format for manifest version.
```

#### [Good example](#tab/good/)

```yaml
Id: microsoft.teams
Publisher: Microsoft Corporation
Name: Microsoft Teams
Version: 1.3.0.4461
License: Copyright (c) Microsoft Corporation. All rights reserved.
LicenseUrl: https://docs.microsoft.com/en-us/MicrosoftTeams/assign-teams-licenses
InstallerType: exe
Installers:
  - Arch: x64
    Url: https://statics.teams.cdn.office.net/production-windows-x64/1.3.00.4461/Teams_windows_x64.exe
    Sha256: 712f139d71e56bfb306e4a7b739b0e1109abb662dfa164192a5cfd6adb24a4e1
ManifestVersion: 0.1.0
```

#### [Better example](#tab/better/)

```yaml
Id: microsoft.teams
Publisher: Microsoft Corporation
Name: Microsoft Teams
Version: 1.3.0.4461
AppMoniker: teams
MinOSVersion: 10.0.0.0
Description: The hub for teamwork in Microsoft 365
Homepage: https://www.microsoft.com/microsoft/teams
License: Copyright (c) Microsoft Corporation. All rights reserved.
LicenseUrl: https://docs.microsoft.com/en-us/MicrosoftTeams/assign-teams-licenses
InstallerType: exe
Installers:
  - Arch: x64
    Url: https://statics.teams.cdn.office.net/production-windows-x64/1.3.00.4461/Teams_windows_x64.exe
    Sha256: 712f139d71e56bfb306e4a7b739b0e1109abb662dfa164192a5cfd6adb24a4e1
ManifestVersion: 0.1.0
```

* * *

> [!NOTE]
> if your installer is an .exe and it was built using Nullsoft or Inno, you may specify those values instead. When Nullsoft or Inno are specified, the client will automatically set the silent and silent with progress install behaviors for the installer.

## Installer switches

You can often figure out what silent `Switches` are available for an installer by passing in a `-?` to the installer from the command line. Here are some common silent `Switches` that can be used for different installer types.

| Installer | Command  | Documentation |  
| :--- | :-- | :--- |  
| MSI | `/q` | [MSI Command-Line Options](/windows/win32/msi/command-line-options) |
| InstallShield | `/s`  | [InstallShield Command-Line Parameters](https://docs.flexera.com/installshield19helplib/helplibrary/IHelpSetup_EXECmdLine.htm) |
| Inno Setup | `/SILENT or /VERYSILENT` | [Inno Setup documentation](https://jrsoftware.org/ishelp/) |
| Nullsoft | `/S` | [Nullsoft Silent Installers/Uninstallers](https://nsis.sourceforge.io/Docs/Chapter4.html#silent) |

## Tips and best practices

* For the best customer experience when finding and installing your software, we recommend that you include as many optional items beyond the required schema as possible. For example, the `AppMoniker` field is optional. However, if you include this field, customers will see results associated with the `AppMoniker` value when performing the [search](../winget/search.md) command (for example, **vscode** for **Visual Studio Code**). If there is only one app with the specified `AppMoniker` value, customers can install your application by specifying the moniker rather than the fully qualified ID.
* The `Id` must be unique. You cannot have multiple submissions with the same package identifier. Avoid spaces, because this will require users to put quotation marks around the `Id` when using the [winget](../index.md) client.
* Avoid creating multiple publisher folders. For example, do not create "Contoso Ltd" if there is already a "Contoso" folder. Also avoid spaces when creating folders.
* All packages should be submitted with a silent install if possible. If you have an executable that does not support a silent install, the user experience will be diminished.
* Limit the length of strings in your manifest to 100 characters before a line break.
* When more than one installer type exists for the specified version of the package, an instance of `InstallerType` can be placed under each of the `Installers`.
