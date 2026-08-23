---
title: winget settings command
description: Opens settings or sets administrator settings for Windows Package Manager.
ms.date: 08/06/2026
ms.topic: article
ms.localizationpriority: medium
---

# settings command (winget)

The **settings** command of the [winget](index.md) tool opens settings in the default JSON text editor. If no editor is configured, it opens settings in Notepad. For available settings, see [https://aka.ms/winget-settings](https://aka.ms/winget-settings).

This command can also be used to set administrator settings by providing the **--enable** or **--disable** arguments, or by using the **export**, **set**, and **reset** subcommands.

## Usage

Launch your default JSON editing tool: `winget settings`

Manage admin settings: `winget settings [<command>] [<options>]`

The following command aliases are available: `config`

![Screenshot of the Windows Package Manager Settings.](images/settings.png)

## Sub-commands

| Sub-command | Description |
|-------------|-------------|
| **export** | Export settings as JSON. |
| **set** | Sets the value of an admin setting. |
| **reset** | Resets an admin setting to its default value. |

## Options

| Option | Description |
|--------|-------------|
| **--enable** | Enables the specific administrator setting. |
| **--disable** | Disables the specific administrator setting. |
| **-?, --help** | Shows help about the selected command. |
| **--wait** | Prompts the user to press any key before exiting. |
| **--logs, --open-logs** | Open the default logs location. |
| **--verbose, --verbose-logs** | Enables verbose logging for winget. |
| **--nowarn, --ignore-warnings** | Suppresses warning outputs. |
| **--disable-interactivity** | Disable interactive prompts. |
| **--proxy** | Set a proxy to use for this execution. |
| **--no-proxy** | Disable the use of proxy for this execution. |

## Sub-command details

### export

Usage: `winget settings export [<options>]`

Exports settings as JSON.

### set

Usage: `winget settings set [--setting] <setting> [--value] <value> [<options>]`

| Argument | Description |
|--------|-------------|
| **--setting** | Name of the setting to modify. |
| **--value** | Value to set for the setting. |

### reset

Usage: `winget settings reset [[--setting] <setting>] [<options>]`

| Argument or option | Description |
|--------|-------------|
| **--setting** | Name of the setting to modify. |
| **-r, --recurse, --all** | Resets all admin settings. |

## Updating settings

When you launch the settings for the first time, there will be no settings specified. At the top of the JSON we provide a link to [https://aka.ms/winget-settings](https://aka.ms/winget-settings) where you can discover the latest experimental features and settings.

We have also defined a schema for the settings file. This allows you to use TAB to discover settings and syntax if your JSON editor supports JSON schemas.

### Source

The `source` settings involve configuration to the WinGet source.

```json
    "source": {
        "autoUpdateIntervalInMinutes": 3
    },
```

#### autoUpdateIntervalInMinutes

A positive integer represents the update interval in minutes. The check for updates only happens when a source is used. A zero will disable the check for updates to a source. Any other values are invalid.

- Disable: 0
- Default: 5

To manually update the source use `winget source update`.

### Visual

The `visual` settings involve visual elements that are displayed by WinGet.

```json
    "visual": {
        "progressBar": "accent"
    },
```

#### progressBar

Color of the progress bar that WinGet displays when not specified by arguments.

- accent (default)
- retro
- rainbow

### Install behavior

The `installBehavior` settings affect the default behavior of installing and upgrading (where applicable) packages.

#### Preferences and requirements

Some of the settings are duplicated under `preferences` and `requirements`. `preferences` affect how the various available options are sorted when choosing the one to act on. For instance, the default scope of package installs is for the current user, but if that is not an option then a machine-level installer will be chosen. `requirements` filter the options, potentially resulting in an empty list and a failure to install. In the previous example, a user scope requirement would result in no applicable installers and an error.

Any arguments passed on the command line will effectively override the matching `requirement` setting for the duration of that command.

#### Scope

The `scope` behavior affects the choice between installing a package for the current user or for the entire machine. The matching parameter is `--scope`, and uses the same values (`user` or `machine`).

```json
    "installBehavior": {
        "preferences": {
            "scope": "user"
        }
    },
```

#### Locale

The `locale` behavior affects the choice of installer based on installer locale. The matching parameter is `--locale`, and uses a BCP 47 language tag.

```json
    "installBehavior": {
        "preferences": {
            "locale": [ "en-US", "fr-FR" ]
        }
    },
```

### Telemetry

The `telemetry` settings control whether winget writes ETW events that may be sent to Microsoft on a default installation of Windows.

See [details on telemetry](https://github.com/microsoft/winget-cli/blob/master/README.md#datatelemetry) and our [primary privacy statement](https://github.com/microsoft/winget-cli/blob/master/PRIVACY.md).

#### disable

```json
    "telemetry": {
        "disable": true
    },
```

If set to true, the `telemetry.disable` setting will prevent any event from being written by the program.

### Network

The `network` settings influence how winget uses the network to retrieve packages and metadata.

#### Downloader

The `downloader` setting controls which code is used when downloading packages. The default is `default`, which may be any of the options based on our determination. `wininet` uses the [WinINet](https://docs.microsoft.com/windows/win32/wininet/about-wininet) APIs, while `do` uses the [Delivery Optimization](https://support.microsoft.com/windows/delivery-optimization-in-windows-10-0656e53c-15f2-90de-a87a-a2172c94cf6d) service.

```json
   "network": {
       "downloader": "do"
   }
```

## Enabling experimental features

To discover which experimental features are available, go to [https://aka.ms/winget-settings](https://aka.ms/winget-settings) where you can see the experimental features available to you.

## Related topics

* [Use the winget tool to install and manage applications](index.md)
