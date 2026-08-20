---
title: mcp Command
description: Displays MCP information and manages extended features.
ms.date: 2026-08-06
ms.topic: overview
ms.localizationpriority: medium
---

# mcp command (winget)

The **mcp** command of the [winget](index.md) tool displays Model Context Protocol (MCP) information for Windows Package Manager.

Live help for **winget mcp** currently exposes enable and disable switches for extended features.

## Usage

`winget mcp [<options>]`

## Options

| Option | Description |
|--------|-------------|
| **--enable** | Enable extended features. Requires store access. |
| **--disable** | Disable extended features. Requires store access. |
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
* [features command](features.md)
