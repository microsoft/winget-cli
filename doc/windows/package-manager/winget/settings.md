---
title: winget settings command
description: Provides customizations for the Windows Package Manager.
ms.date: 05/05/2021
ms.topic: article
ms.localizationpriority: medium
---

# help command (winget)

[!INCLUDE [preview-note](../../includes/package-manager-preview.md)]

The **settings** command of the [winget](index.md) tool allows you to customize your Windows Package Manager client experience.  You can change defaults and try out experimental features that are enabled in your client.
The **settings** command will launch your default MD editor.  Windows by default will launch Notepad as an option.  We recommend using a tool like [Visual Studio code](https://code.visualstudio.com/).  

>[!NOTE]
>You can easily install Visual Studio Code, by typing `winget install Microsoft.VisualStudioCode`

## Usage

Launch your default JSON editing tool: `winget settings`

![Screenshot of the Windows Package Manager Settings.](images\settings.png)

When you launch the settings for the first time, there will be no settings specified. At the top of the JSON we provide a [link](https://aka.ms/winget-settings) where you can discover the latest experimental features and settings.

We have also defined a schema for the settings file.  This allows you to use TAB to discover settings and syntax if your JSON editor supports JSON schemas.

## Updating Settings

The following settings are available for the 1.0 release of the Windows Package Manager.

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

To manually update the source use `winget source update`

### Visual

The `visual` settings involve visual elements that are displayed by WinGet

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

### Install Behavior

The `installBehavior` settings affect the default behavior of installing and upgrading (where applicable) packages.

#### Preferences and Requirements

Some of the settings are duplicated under `preferences` and `requirements`. `preferences` affect how the various available options are sorted when choosing the one to act on.  For instance, the default scope of package installs is for the current user, but if that is not an option then a machine level installer will be chosen. `requirements` filter the options, potentially resulting in an empty list and a failure to install. In the previous example, a user scope requirement would result in no applicable installers and an error.

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

The `locale` behavior affects the choice of installer based on installer locale. The matching parameter is `--locale`, and uses bcp47 language tag.

```json
    "installBehavior": {
        "preferences": {
            "locale": [ "en-US", "fr-FR" ]
        }
    },
```

### Telemetry

The `telemetry` settings control whether winget writes ETW events that may be sent to Microsoft on a default installation of Windows.

See [details on telemetry](https://github.com/microsoft/winget-cli/blob/master/README.md#datatelemetry), and our [primary privacy statement](https://github.com/microsoft/winget-cli/blob/master/privacy.md).

#### disable

```
    "telemetry": {
        "disable": true
    },
```

If set to true, the `telemetry.disable` setting will prevent any event from being written by the program.

### Network

The `network` settings influence how winget uses the network to retrieve packages and metadata.

#### Downloader

The `downloader` setting controls which code is used when downloading packages. The default is `default`, which may be any of the options based on our determination.
`wininet` uses the [WinINet](https://docs.microsoft.com/en-us/windows/win32/wininet/about-wininet) APIs, while `do` uses the
[Delivery Optimization](https://support.microsoft.com/en-us/windows/delivery-optimization-in-windows-10-0656e53c-15f2-90de-a87a-a2172c94cf6d) service.

```json
   "network": {
       "downloader": "do"
   }
```

## Enabling Experimental features

To discover which experimental features are available, go to [https://aka.ms/winget-settings](https://aka.ms/winget-settings) where you can see the experimental features available to you.

