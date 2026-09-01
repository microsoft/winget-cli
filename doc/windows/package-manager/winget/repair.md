---
title: repair Command
description: Repairs the selected package.
ms.date: 08/06/2026
ms.topic: overview
ms.localizationpriority: medium
---

# repair command (winget)

The **repair** command of the [winget](index.md) tool repairs the selected package, either found by searching the installed packages list or directly from a manifest.

## Usage

`winget repair [[-q] <query>] [<options>]`

The following command aliases are available: `fix`

## Arguments

| Argument | Description |
|--------|-------------|
| **-q, --query** | The query used to search for a package. |

## Options

| Option | Description |
|--------|-------------|
| **-m, --manifest** | The path to the manifest of the package. |
| **--id** | Filter results by id. |
| **--name** | Filter results by name. |
| **--moniker** | Filter results by moniker. |
| **-v, --version** | The version to act upon. |
| **--product-code** | Filters using the product code. |
| **-a, --architecture** | Select the architecture. |
| **--scope** | Select installed package scope filter (user or machine). |
| **-s, --source** | Find package using the specified source. |
| **-i, --interactive** | Request interactive installation; user input may be needed. |
| **-h, --silent** | Request silent installation. |
| **-o, --log** | Log location (if supported). |
| **--ignore-local-archive-malware-scan** | Ignore the malware scan performed as part of installing an archive type package from local manifest. |
| **--accept-source-agreements** | Accept all source agreements during source operations. |
| **--accept-package-agreements** | Accept all license agreements for packages. |
| **--locale** | Locale to use (BCP47 format). |
| **--header** | Optional Windows-Package-Manager REST source HTTP header. |
| **--authentication-mode** | Specify authentication window preference (`silent`, `silentPreferred`, or `interactive`). |
| **--authentication-account** | Specify the account to be used for authentication. |
| **--force** | Direct run the command and continue with non security related issues. |
| **--ignore-security-hash** | Ignore the installer hash check failure. |
| **-e, --exact** | Find package using exact match. |
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
* [uninstall command](uninstall.md)
