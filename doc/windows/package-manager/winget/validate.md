---
title: winget validate Command
description: Validates a manifest file for submitting software to the Microsoft Community Package Manifest Repository on GitHub.
ms.date: 04/28/2020
ms.topic: article
ms.localizationpriority: medium
---

# validate command (winget)

The **validate** command of the [winget](index.md) tool validates a [manifest](../package/manifest.md) for submitting software to the **Microsoft Community Package Manifest Repository** on GitHub. The manifest must be a YAML file that follows the [specification](https://github.com/microsoft/winget-pkgs/blob/master/doc/manifest/README.md).

## Usage

`winget validate [--manifest] <manifest> [<options>]`

## Arguments

The following arguments are available.

| Argument  | Description |
|--------------|-------------|
| **--manifest** |  The path to the manifest to be validated. |

## Options

The options allow you to customize the export experience to meet your needs.

| Option | Description |
|--------|-------------|
| **-?, --help** | Get additional help on this command. |
| **--wait** | Prompts the user to press any key before exiting. |
| **--logs, --open-logs** | Open the default logs location. |
| **--verbose, --verbose-logs** | Enables verbose logging for winget. |
| **--disable-interactivity** | Disable interactive prompts. |

## Related topics

* [Use the winget tool to install and manage applications](index.md)
* [Submit packages to Windows Package Manager](../package/index.md)
