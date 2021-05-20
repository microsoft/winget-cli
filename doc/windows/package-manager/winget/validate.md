---
title: winget validate Command
description: Validates a manifest file for submitting software to the Microsoft Community Package Manifest Repository on GitHub.
ms.date: 04/28/2020
ms.topic: article
ms.localizationpriority: medium
---

# validate command (winget)

[!INCLUDE [preview-note](../../includes/package-manager-preview.md)]

The **validate** command of the [winget](index.md) tool validates a [manifest](../package/manifest.md) for submitting software to the **Microsoft Community Package Manifest Repository** on GitHub. The manifest must be a YAML file that follows the [specification](https://github.com/microsoft/winget-pkgs/YamlSpec.md).

## Usage

`winget validate [--manifest] \<path>`

## Arguments

The following arguments are available.

| Argument  | Description |
|--------------|-------------|
| **--manifest** |  the path to the manifest to be validated. |
| **-?, --help** |  get additional help on this command |

## Related topics

* [Use the winget tool to install and manage applications](index.md)
* [Submit packages to Windows Package Manager](../package/index.md)
