# WinGet CLI Settings

You can configure WinGet by editing the `settings.json` file. Running `winget settings` will open the file in the default json editor; if no editor is configured, Windows will prompt for you to select an editor, and Notepad is a sensible option if you have no other preference.

## File Location

Settings file is located in %LOCALAPPDATA%\Packages\Microsoft.DesktopAppInstaller_8wekyb3d8bbwe\LocalState\settings.json

If you are using the non-packaged WinGet version by building it from source code, the file will be located under %LOCALAPPDATA%\Microsoft\WinGet\Settings\settings.json

## Source

The `source` settings involve configuration to the WinGet source.

```json
    "source": {
        "autoUpdateIntervalInMinutes": 3
    },
``` 

### autoUpdateIntervalInMinutes

A positive integer represents the update interval in minutes. The check for updates only happens when a source is used. A zero will disable the check for updates to a source. Any other values are invalid.

- Disable: 0
- Default: 5

To manually update the source use `winget source update`

## Visual

The `visual` settings involve visual elements that are displayed by WinGet

```json
    "visual": {
        "progressBar": "accent"
    },
```

### progressBar

Color of the progress bar that WinGet displays when not specified by arguments. 

- accent (default)
- retro
- rainbow

## Install Behavior

The `installBehavior` settings affect the default behavior of installing and upgrading (where applicable) packages.

### Preferences and Requirements

Some of the settings are duplicated under `preferences` and `requirements`. `preferences` affect how the various available options are sorted when choosing the one to act on.  For instance, the default scope of package installs is for the current user, but if that is not an option then a machine level installer will be chosen. `requirements` filter the options, potentially resulting in an empty list and a failure to install. In the previous example, a user scope requirement would result in no applicable installers and an error.

Any arguments passed on the command line will effectively override the matching `requirement` setting for the duration of that command.

### Scope

The `scope` behavior affects the choice between installing a package for the current user or for the entire machine. The matching parameter is `--scope`, and uses the same values (`user` or `machine`).

```json
    "installBehavior": {
        "preferences": {
            "scope": "user"
        }
    },
```

### Locale

The `locale` behavior affects the choice of installer based on installer locale. The matching parameter is `--locale`, and uses bcp47 language tag.

```json
    "installBehavior": {
        "preferences": {
            "locale": [ "en-US", "fr-FR" ]
        }
    },
```
### Architectures

The `architectures` behavior affects what architectures will be selected when installing a package. The matching parameter is `--architecture`. Note that only architectures compatible with your system can be selected.

```json
    "installBehavior": {
        "preferences": {
            "architectures": ["x64", "arm64"]
        }
    },
```

## Telemetry

The `telemetry` settings control whether winget writes ETW events that may be sent to Microsoft on a default installation of Windows.

See [details on telemetry](../README.md#datatelemetry), and our [primary privacy statement](../privacy.md).

### disable

```json
    "telemetry": {
        "disable": true
    },
```

If set to true, the `telemetry.disable` setting will prevent any event from being written by the program.

## Logging

The `logging` settings control the level of detail in log files. `--verbose-logs` will override this setting and always creates a verbose log.
Defaults to `info` if value is not set or is invalid

### level

```json
    "logging": {
        "level": ["verbose", "info", "warning", "error", "critical"]
    },
```

## Network

The `network` settings influence how winget uses the network to retrieve packages and metadata.

### Downloader

The `downloader` setting controls which code is used when downloading packages. The default is `default`, which may be any of the options based on our determination.
`wininet` uses the [WinINet](https://docs.microsoft.com/windows/win32/wininet/about-wininet) APIs, while `do` uses the
[Delivery Optimization](https://support.microsoft.com/windows/delivery-optimization-in-windows-10-0656e53c-15f2-90de-a87a-a2172c94cf6d) service.

The `doProgressTimeoutInSeconds` setting updates the number of seconds to wait without progress before fallback. The default number of seconds is 60, minimum is 1 and the maximum is 600. 

```json
   "network": {
       "downloader": "do",
       "doProgressTimeoutInSeconds": 60
   }
```

## Experimental Features

To allow work to be done and distributed to early adopters for feedback, settings can be used to enable "experimental" features. 

The `experimentalFeatures` settings involve the configuration of these "experimental" features. Individual features can be enabled under this node. The example below shows sample experimental features.

```json
   "experimentalFeatures": {
       "experimentalCmd": true,
       "experimentalArg": false
   },
```

### directMSI

This feature enables the Windows Package Manager to directly install MSI packages with the MSI APIs rather than through msiexec. 
Note that when silent installation is used this is already in affect, as MSI packages that require elevation will fail in that scenario without it. 
You can enable the feature as shown below.

```json
   "experimentalFeatures": {
       "directMSI": true
   },
```
### Dependencies

Experimental feature with the aim of managing dependencies, as of now it only shows package dependency information. You can enable the feature as shown below.

```json
   "experimentalFeatures": {
       "dependencies": true
   },
```
