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

For a complete list and descriptions of items in a manifest, see the [manifest specification](https://github.com/microsoft/winget-cli/blob/master/doc/ManifestSpecv1.0.md) in the [https://github.com/microsoft/winget-cli](https://github.com/microsoft/winget-cli) repository.

### Minimal required schema

#### [Minimal required schema](#tab/minschema/)

As specified in the [singleton JSON schema](https://github.com/microsoft/winget-cli/blob/master/schemas/JSON/manifests/v1.0.0/manifest.singleton.1.0.0.json),
only a number of fields are required.  The minimal supported YAML file would look like the example below. The singleton format is only valid for packages containing
a single installer and a single locale. If more than one installer or locale is provided, the multiple YAML file format and schema must be used.

The partitioning scheme was added to help with GitHub's UX. Folders with thousands of children do not render well in the browser.


```yaml
PackageIdentifier:  # Publisher.package format.
PackageVersion:     # Version numbering format.
PackageLocale:      # BCP 47 format (e.g. en-US)
Publisher:          # The name of the publisher.
PackageName:        # The name of the application.
License:            # The license of the application.
ShortDescription:   # The description of the application.
Installers: 
 - Architecture:    # Enumeration of supported architectures.
   InstallerType:   # Enumeration of supported installer types (exe, msi, msix, inno, wix, nullsoft, appx).
   InstallerUrl:    # Path to download installation file.
   InstallerSha256: # SHA256 calculated from installer.
ManifestType:       # The manifest file type
ManifestVersion: 1.0.0
```

#### [Example](#tab/minexample/)

Path: manifests / m / Microsoft / WindowsTerminal / 1.6.10571.0 / WindowsTerminal.yaml

```YAML
PackageIdentifier: Microsoft.WindowsTerminal
PackageVersion: 1.6.10571.0
PackageLocale: en-US
Publisher: Microsoft
PackageName: Windows Terminal
License: MIT
ShortDescription: The new Windows Terminal, a tabbed command line experience for Windows.
Installers: 
 - Architecture: x64
   InstallerType: msix
   InstallerUrl: https://github.com/microsoft/terminal/releases/download/v1.6.10571.0/Microsoft.WindowsTerminal_1.6.10571.0_8wekyb3d8bbwe.msixbundle
   InstallerSha256: 092aa89b1881e058d31b1a8d88f31bb298b5810afbba25c5cb341cfa4904d843
   SignatureSha256: e53f48473621390c8243ada6345826af7c713cf1f4bbbf0d030599d1e4c175ee
ManifestType: singleton
ManifestVersion: 1.0.0
```

#### Multiple File Example
In order to provide the best user experience, manifests should contain as much meta-data as possible. In order to separate concerns for validating installers
and providing localized meta-data manifests will be split into multiple files. The minimum number of YAML files for this kind of manifest is three. Additional
locales should also be provided. 
* A [version](https://github.com/microsoft/winget-cli/blob/master/schemas/JSON/manifests/v1.0.0/manifest.version.1.0.0.json) file
* The [default locale](https://github.com/microsoft/winget-cli/blob/master/schemas/JSON/manifests/v1.0.0/manifest.defaultLocale.1.0.0.json) file
* An [installer](https://github.com/microsoft/winget-cli/blob/master/schemas/JSON/manifests/v1.0.0/manifest.installer.1.0.0.json) file
* Additional [locale](https://github.com/microsoft/winget-cli/blob/master/schemas/JSON/manifests/v1.0.0/manifest.locale.1.0.0.json) files

The example below shows many optional meta-data fields and multiple locales. Note the default locale has more requirements than additional locales. In the show
command, any required fields that aren't provided for additional locales will display fields from the default locale.

Path: manifests / m / Microsoft / WindowsTerminal / 1.6.10571.0 / Microsoft.WindowsTerminal.yaml

```YAML
PackageIdentifier: "Microsoft.WindowsTerminal"
PackageVersion: "1.6.10571.0"
DefaultLocale: "en-US"
ManifestType: "version"
ManifestVersion: "1.0.0"
```

Path: manifests / m / Microsoft / WindowsTerminal / 1.6.10571.0 / Microsoft.WindowsTerminal.locale.en-US.yaml

```YAML
PackageIdentifier: "Microsoft.WindowsTerminal"
PackageVersion: "1.6.10571.0"
PackageLocale: "en-US"
Publisher: "Microsoft"
PublisherURL: "https://www.microsoft.com/"
PrivacyURL: "https://privacy.microsoft.com/"
PackageName: "Windows Terminal"
PackageURL: "https://docs.microsoft.com/windows/terminal/"
License: "MIT"
LicenseURL: "https://github.com/microsoft/terminal/blob/master/LICENSE"
ShortDescription: "The new Windows Terminal, a tabbed command line experience for Windows."
Tags: 
- "Console"
- "Command-Line"
- "Shell"
- "Command-Prompt"
- "PowerShell"
- "WSL"
- "Developer-Tools"
- "Utilities"
- "cli"
- "cmd"
- "ps"
- "terminal"
ManifestType: "defaultLocale"
ManifestVersion: "1.0.0"
```

Path: manifests / m / Microsoft / WindowsTerminal / 1.6.10571.0 / Microsoft.WindowsTerminal.locale.fr-FR.yaml

```YAML
PackageIdentifier: "Microsoft.WindowsTerminal"
PackageVersion: "1.6.10571.0"
PackageLocale: "fr-FR"
Publisher: "Microsoft"
ShortDescription: "Le nouveau terminal Windows, une expérience de ligne de commande à onglets pour Windows."
ManifestType: "locale"
ManifestVersion: "1.0.0"
```

Path: manifests / m / Microsoft / WindowsTerminal / 1.6.10571.0 / Microsoft.WindowsTerminal.installer.yaml

```YAML
PackageIdentifier: "Microsoft.WindowsTerminal"
PackageVersion: "1.6.10571.0"
Platform: 
 - "Windows.Desktop"
MinimumOSVersion: "10.0.18362.0"
InstallerType: "msix"
InstallModes: 
 - "silent"
PackageFamilyName: "Microsoft.WindowsTerminal_8wekyb3d8bbwe"
Installers: 
 - Architecture: "x64"
   InstallerUrl: "https://github.com/microsoft/terminal/releases/download/v1.6.10571.0/Microsoft.WindowsTerminal_1.6.10571.0_8wekyb3d8bbwe.msixbundle"
   InstallerSha256: 092aa89b1881e058d31b1a8d88f31bb298b5810afbba25c5cb341cfa4904d843
   SignatureSha256: e53f48473621390c8243ada6345826af7c713cf1f4bbbf0d030599d1e4c175ee
 - Architecture: "arm64"
   InstallerUrl: "https://github.com/microsoft/terminal/releases/download/v1.6.10571.0/Microsoft.WindowsTerminal_1.6.10571.0_8wekyb3d8bbwe.msixbundle"
   InstallerSha256: 092aa89b1881e058d31b1a8d88f31bb298b5810afbba25c5cb341cfa4904d843
   SignatureSha256: e53f48473621390c8243ada6345826af7c713cf1f4bbbf0d030599d1e4c175ee
 - Architecture: "x86"
   InstallerUrl: "https://github.com/microsoft/terminal/releases/download/v1.6.10571.0/Microsoft.WindowsTerminal_1.6.10571.0_8wekyb3d8bbwe.msixbundle"
   InstallerSha256: 092aa89b1881e058d31b1a8d88f31bb298b5810afbba25c5cb341cfa4904d843
   SignatureSha256: e53f48473621390c8243ada6345826af7c713cf1f4bbbf0d030599d1e4c175ee
ManifestType: "installer"
ManifestVersion: "1.0.0"
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

* The package identifier must be unique.  You cannot have multiple submissions with the same package identifier. Only one pull request per package version is allowed.

* Avoid creating multiple publisher folders.  For example, do not create "Contoso Ltd." if there is already a "Contoso" folder.

* All tools must support a silent install.  If you have an executable that does not support a silent install, then we cannot provide that tool at this time.

* Provide as many fields as possible.  The more meta-data you provide the better the user experience will be. In some cases, the fields may not yet be supported
by the Windows Package Manager client (winget.exe). For example, the `AppMoniker` field is optional. However, if you include this field, customers will see results associated with the `AppMoniker` value when performing the [search](../winget/search.md) command (for example, **vscode** for **Visual Studio Code**). If there is only one app with the specified `AppMoniker` value, customers can install your application by specifying the moniker rather than the fully qualified package identifier.

* The length of strings in this specification should be limited to 100 characters before a line break.

* The "PackageName" should match the entry made in Add / Remove Programs to help the correlation with manifests to support **export**, and **upgrade**.

* The "Publisher" should match the entry made in Add / Remove Programs to help the correlation with manifests to support **export**, and **upgrade**.

* Package installers in MSI format use [Product Codes](https://docs.microsoft.com/en-us/windows/win32/msi/product-codes) to uniquely identify applications. The product code for a given version of a package should be included in the manifest to help ensure the best **upgrade** experience.

* Limit the length of strings in your manifest to 100 characters before a line break.
* When more than one installer type exists for the specified version of the package, an instance of `InstallerType` can be placed under each of the `Installers`.
