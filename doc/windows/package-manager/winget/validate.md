---
title: winget validate Command
description: Validates a manifest file.
ms.date: 2026-08-06
ms.topic: article
ms.localizationpriority: medium
---

# validate command (winget)

The **validate** command of the [winget](index.md) tool validates a [manifest](../package/manifest.md) using a strict set of guidelines. This is intended to enable you to check your manifest before submitting it to a repository.

## Usage

`winget validate [--manifest] <manifest> [<options>]`

## Arguments

The following arguments are available.

| Argument | Description |
|--------------|-------------|
| **--manifest** | The path to the manifest to be validated. |

## Options

The following options are available.

| Option | Description |
|--------|-------------|
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
* [Submit packages to Windows Package Manager](../package/index.md)
