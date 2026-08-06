---
title: pin Command
description: Manages package pins.
ms.date: 2026-08-06
ms.topic: overview
ms.localizationpriority: medium
---

# pin command (winget)

The **pin** command of the [winget](index.md) tool manages package pins. A pin can limit Windows Package Manager from upgrading a package to specific ranges of versions, or it can prevent the package from being upgraded altogether.

## Usage

`winget pin [<command>] [<options>]`

## Sub-commands

| Sub-command | Description |
|-------------|-------------|
| **add** | Add a new pin. |
| **remove** | Remove a package pin. |
| **list** | List current pins. |
| **reset** | Reset pins. |

## Options

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

## add

Adds a new pin.

Usage: `winget pin add [[-q] <query>] [<options>]`

| Option | Description |
|--------|-------------|
| **--id** | Filter results by id. |
| **--name** | Filter results by name. |
| **--moniker** | Filter results by moniker. |
| **--tag** | Filter results by tag. |
| **--cmd, --command** | Filter results by command. |
| **-e, --exact** | Find package using exact match. |
| **-v, --version** | Version to which to pin the package. The wildcard `*` can be used as the last version part. |
| **-s, --source** | Find package using the specified source. |
| **--header** | Optional Windows-Package-Manager REST source HTTP header. |
| **--authentication-mode** | Specify authentication window preference (`silent`, `silentPreferred`, or `interactive`). |
| **--authentication-account** | Specify the account to be used for authentication. |
| **--accept-source-agreements** | Accept all source agreements during source operations. |
| **--force** | Direct run the command and continue with non security related issues. |
| **--blocking** | Block from upgrading until the pin is removed, preventing override arguments. |
| **--installed** | Pin a specific installed version. |

## list

Lists all current pins, or full details of a specific pin.

Usage: `winget pin list [[-q] <query>] [<options>]`

## remove

Removes a specific package pin.

Usage: `winget pin remove [[-q] <query>] [<options>]`

Additional option: **--installed** targets a specific installed version.

## reset

Resets all existing pins.

Usage: `winget pin reset [<options>]`

| Option | Description |
|--------|-------------|
| **--force** | Direct run the command and continue with non security related issues. |
| **-s, --source** | Find package using the specified source. |

## Related topics

* [Use the winget tool to install and manage applications](index.md)
* [upgrade command](upgrade.md)
