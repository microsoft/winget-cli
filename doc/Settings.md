# WinGet CLI Settings

You can configure WinGet by editing the `settings.json` file. Running `winget settings` will open the file in the default json editor, if no editor is configured, notepad.exe is used.

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

## Telemetry

The `telemetry` settings control whether winget writes ETW events that may be sent to Microsoft on a default installation of Windows.

See [details on telemetry](../README.md#datatelemetry), and our [primary privacy statement](../privacy.md).

### disable

```
    "telemetry": {
        "disable": true
    },
```

If set to true, the `telemetry.disable` setting will prevent any event from being written by the program.

## Experimental Features

To allow work to be done and distributed to early adopters for feedback, settings can be used to enable "experimental" features. 

The `experimentalFeatures` settings involve the configuration of these "experimental" features. Individual features can be enabled under this node. The example below shows sample experimental features.

```json
   "experimentalFeatures": {
       "experimentalCmd": true,
       "experimentalArg": false
   },
```

### experimentalMSStore

Microsoft Store App support in WinGet is currently implemented as an experimental feature. It supports a curated list of utility apps from Microsoft Store. You can enable the feature as shown below.

```json
   "experimentalFeatures": {
       "experimentalMSStore": true
   },
```

### list

While work is in progress on list, the command is hidden behind a feature toggle. One can enable it as below:

```json
   "experimentalFeatures": {
       "list": true
   },
```

### upgrade

While work is in progress on upgrade, the command is hidden behind a feature toggle. One can enable it as below:

```json
   "experimentalFeatures": {
       "upgrade": true
   },
```

### uninstall

While work is in progress on uninstall, the command is hidden behind a feature toggle. One can enable it as below:

```json
   "experimentalFeatures": {
       "uninstall": true
   },
```

### restSource

While work is in progress for rest source support, the feature is hidden behind a feature toggle. Enabling this will not change how client works currently and will allow testing any additional rest sources added. One can enable it as below:

```json
   "experimentalFeatures": {
       "restSource": true
   },
```
