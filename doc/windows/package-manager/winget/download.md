---
title: download Command
description: Downloads the installer from a given package.
ms.date: 08/06/2026
ms.topic: overview
ms.localizationpriority: medium
---

# download command (winget)

The **download** command of the [winget](index.md) tool downloads the installer from the selected package, either found by searching a configured source or directly from a manifest. By default, it downloads the appropriate installer to the user's Downloads folder.

## Usage

`winget download [[-q] <query>] [<options>]`

## Arguments

| Argument | Description |
|--------|-------------|
| **-q, --query** | The query used to search for a package. |

## Options

| Option | Description |
|--------|-------------|
| **-d, --download-directory** | Directory where the installers are downloaded to. |
| **-m, --manifest** | The path to the manifest of the package. |
| **--id** | Filter results by id. |
| **--name** | Filter results by name. |
| **--moniker** | Filter results by moniker. |
| **-v, --version** | Use the specified version; default is the latest version. |
| **-s, --source** | Find package using the specified source. |
| **--scope** | Select install scope (user or machine). |
| **-a, --architecture** | Select the architecture. |
| **--installer-type** | Select the installer type. |
| **-e, --exact** | Find package using exact match. |
| **--locale** | Locale to use (BCP47 format). |
| **--ignore-security-hash** | Ignore the installer hash check failure. |
| **--skip-dependencies** | Skips processing package dependencies and Windows features. |
| **--header** | Optional Windows-Package-Manager REST source HTTP header. |
| **--authentication-mode** | Specify authentication window preference (`silent`, `silentPreferred`, or `interactive`). |
| **--authentication-account** | Specify the account to be used for authentication. |
| **--accept-package-agreements** | Accept all license agreements for packages. |
| **--accept-source-agreements** | Accept all source agreements during source operations. |
| **--skip-license, --skip-microsoft-store-package-license** | Skips retrieving Microsoft Store package offline license. |
| **--platform** | Select the target platform. |
| **--os-version** | Target OS version. |
| **-?, --help** | Shows help about the selected command. |
| **--wait** | Prompts the user to press any key before exiting. |
| **--logs, --open-logs** | Open the default logs location. |
| **--verbose, --verbose-logs** | Enables verbose logging for winget. |
| **--nowarn, --ignore-warnings** | Suppresses warning outputs. |
| **--disable-interactivity** | Disable interactive prompts. |
| **--proxy** | Set a proxy to use for this execution. |
| **--no-proxy** | Disable the use of proxy for this execution. |

## Related topics

* [Use the winget tool to install and manage applications](index.md)
* [install command](install.md)
