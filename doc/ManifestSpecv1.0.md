The Windows Package Manager uses manifests (YAML files) to locate and install packages for Windows users.  This specification provides 
references to JSON schemas as well as best practices.

Table of Contents
----------------------------------
YAML Specification
   1) YAML file name and folder structure
   2) YAML Syntax
   3) Minimal singleton YAML file example
   4) Multiple file YAML example
   5) Best Practices

# YAML Specification

## YAML file name and folder structure
YAML files shall be added to the repository with the following folder structure:
manifests / p / publisher / package / packageVersion / publisher.package.manifestFile.yaml

Example:
manifests / m / Microsoft / WindowsTerminal / 1.6.10571.0 / WindowsTerminal.yaml

* Manifests are partitioned by the first letter of the publisher name (in lower case). For example: m.
* Publisher folder is the name of the company that publishes the tool.  For example: Microsoft.
* The child folder package is the name of the application or tool.  For example: WindowsTerminal.
* The child folder package version is the version of the package. For example: 1.6.10571.0.
* The filename must be a combination of the application name and the current version.  For example: WindowsTerminal.yaml.

The publisher and application folders MUST match the values used to define the Id.  See PackageIdentifier: in the YAML for more detail.
The version in the folder name MUST match the version field value in the YAML file.  See PackageVersion: in the YAML for more detail.

There are two primary types of manifests. A single file manifest (singleton) and a multi-file manifest. 
[JSON schemas](https://github.com/microsoft/winget-cli/tree/master/schemas/JSON/manifests/v1.0.0) have been provided 
to help strongly type attributes and requirements.

## YAML Syntax
Each field in the file must be PascalCased and cannot be duplicated.

## Minimal singleton YAML file example
As specified in the [singleton JSON schema](https://github.com/microsoft/winget-cli/blob/master/schemas/JSON/manifests/v1.0.0/manifest.singleton.1.0.0.json),
only a number of fields are required.  The minimal supported YAML file would look like the example below. The singleton format is only valid for packages containing
a single installer and a single locale. If more than one installer or locale is provided, the multiple YAML file format and schema must be used.

The partitioning scheme was added to help with GitHub's UX. Folders with thousands of children do not render well in the browser.

Path: manifests / m / Microsoft / WindowsTerminal / 1.6.10571.0 / WindowsTerminal.yaml

```YAML
PackageIdentifier: "Microsoft.WindowsTerminal"
PackageVersion: "1.6.10571.0"
PackageLocale: "en-US"
Publisher: "Microsoft"
PackageName: "Windows Terminal"
License: "MIT"
ShortDescription: "The new Windows Terminal, a tabbed command line experience for Windows."
Installers: 
 - Architecture: "x64"
   InstallerType: "msix"
   InstallerUrl: "https://github.com/microsoft/terminal/releases/download/v1.6.10571.0/Microsoft.WindowsTerminal_1.6.10571.0_8wekyb3d8bbwe.msixbundle"
   InstallerSha256: 092aa89b1881e058d31b1a8d88f31bb298b5810afbba25c5cb341cfa4904d843
   SignatureSha256: e53f48473621390c8243ada6345826af7c713cf1f4bbbf0d030599d1e4c175ee
ManifestType: "singleton"
ManifestVersion: "1.0.0"
```

## Multiple file YAML example
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


## Best Practices
The package identifier must be unique.  You cannot have multiple submissions with the same package identifier. Only one pull request per package version is allowed.

Avoid creating multiple publisher folders.  For example, do not create "Contoso Ltd." if there is already a "Contoso" folder.

All tools must support a silent install.  If you have an executable that does not support a silent install, then we cannot provide that tool at this time.

Provide as many fields as possible.  The more meta-data you provide the better the user experience will be. In some cases, the fields may not yet be supported
by the Windows Package Manager client (winget.exe).

The length of strings in this specification should be limited to 100 characters before a line break.
