---
title: configure Command
description: Configures the system into a desired state.
ms.date: 08/06/2026
ms.topic: overview
ms.localizationpriority: medium
---

# configure command (winget)

The **configure** command of the [winget](index.md) tool ensures that the system matches the desired state described by a configuration file. It may download or execute processors in order to achieve that desired state.

## Usage

`winget configure [<command>] [[-f] <file>] [[--module-path] <module-path>] [<options>]`

The following command aliases are available: `configuration`, `dsc`

## Arguments

| Argument | Description |
|--------|-------------|
| **-f, --file** | The path to the configuration file. |
| **--module-path** | Specifies the location on the local computer to store modules. Default `%LOCALAPPDATA%\Microsoft\WinGet\Configuration\Modules`. |

## Sub-commands

| Sub-command | Description |
|-------------|-------------|
| **show** | Shows details of a configuration. |
| **list** | Shows configuration history. |
| **test** | Checks the system against a desired state. |
| **validate** | Validates a configuration file. |
| **export** | Exports configuration resources to a configuration file. |

## Options

| Option | Description |
|--------|-------------|
| **--processor-path** | Specify the path to the configuration processor. |
| **-h, --history** | Select items from history. |
| **--accept-configuration-agreements** | Accepts the configuration warning, preventing an interactive prompt. |
| **--suppress-initial-details** | Suppress showing initial configuration details when possible. |
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

## Sub-command details

### show

Usage: `winget configure show [[-f] <file>] [[--module-path] <module-path>] [<options>]`

Shows details of the provided configuration.

### list

Usage: `winget configure list [<options>]`

Shows the high level details for configurations that have been applied to the system.

Additional options:

| Option | Description |
|--------|-------------|
| **-h, --history** | Select items from history. |
| **-o, --output** | File where the result is to be written. |
| **--remove** | Remove the item from history. |

### test

Usage: `winget configure test [[-f] <file>] [[--module-path] <module-path>] [<options>]`

Checks that the system matches the desired state described by the provided configuration.

### validate

Usage: `winget configure validate [-f] <file> [[--module-path] <module-path>] [<options>]`

Validates a configuration file for correctness.

### export

Usage: `winget configure export [<options>]`

Exports configuration resources to a configuration file. When used with **--all**, it exports all package configurations. When used with **--package-id**, it exports a `WinGetPackage` resource for the given package ID. When used with **--module** and **--resource**, it gets the settings of the resource and exports them to the configuration file. If the output configuration file already exists, the exported configuration resources are appended.

| Option | Description |
|--------|-------------|
| **-o, --output** | File where the result is to be written. |
| **--package-id** | The package identifier to export. |
| **--module** | The module of the resource to export. |
| **--resource** | The configuration resource to export. |
| **--module-path** | Specifies the location on the local computer to store modules. Default `%LOCALAPPDATA%\Microsoft\WinGet\Configuration\Modules`. |
| **--processor-path** | Specify the path to the configuration processor. |
| **-s, --source** | Export packages from the specified source. |
| **--include-versions** | Include package versions in export file. |
| **-r, --recurse, --all** | Exports all package configurations. |
| **--accept-source-agreements** | Accept all source agreements during source operations. |

## Related topics

* [Use the winget tool to install and manage applications](index.md)
* [settings command](settings.md)
