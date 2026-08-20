---
title: dscv3 Command
description: Provides DSC v3 resource commands for WinGet.
ms.date: 2026-08-06
ms.topic: overview
ms.localizationpriority: medium
---

# dscv3 command (winget)

The **dscv3** command of the [winget](index.md) tool provides Desired State Configuration (DSC) v3 resources for configuring WinGet and packages.

## Usage

`winget dscv3 [<command>] [<options>]`

## Sub-commands

| Sub-command | Description |
|-------------|-------------|
| **package** | Manage package state. |
| **source** | Manage source configuration. |
| **user-settings-file** | Manage the user settings file. |
| **admin-settings** | Manage administrator settings. |

## Top-level options

| Option | Description |
|--------|-------------|
| **--manifest** | Get the resource manifest. |
| **-o, --output** | Directory where the results are to be written. |
| **-?, --help** | Shows help about the selected command. |
| **--wait** | Prompts the user to press any key before exiting. |
| **--logs, --open-logs** | Open the default logs location. |
| **--verbose, --verbose-logs** | Enables verbose logging for winget. |
| **--nowarn, --ignore-warnings** | Suppresses warning outputs. |
| **--disable-interactivity** | Disable interactive prompts. |
| **--proxy** | Set a proxy to use for this execution. |
| **--no-proxy** | Disable the use of proxy for this execution. |

## Resource subcommands

Each resource subcommand currently supports the same operational switches.

| Option | Description |
|--------|-------------|
| **--get** | Get the resource state. |
| **--set** | Set the resource state. |
| **--test** | Test the resource state. |
| **--export** | Get all state instances. |
| **--schema** | Get the resource schema. |
| **--manifest** | Get the resource manifest. |
| **-o, --output** | File where the result is to be written. |

These options apply to:

* `winget dscv3 package`
* `winget dscv3 source`
* `winget dscv3 user-settings-file`
* `winget dscv3 admin-settings`

## Related topics

* [Use the winget tool to install and manage applications](index.md)
* [configure command](configure.md)
