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
- Default: 15

To manually update the source use `winget source update`

## Visual

The `visual` settings involve visual elements that are displayed by WinGet

### progressBar

Style of the progress spinner and bar that WinGet displays when not specified by arguments. In addition, all options except `disabled` send [Virtual Terminal progress](https://conemu.github.io/en/AnsiEscapeCodes.html#ConEmu_specific_OSC) updates that any supporting terminal may display visually.

|Value|Description|Release|
|---|---|---|
|`accent` (default)|Use the [Windows Accent color](https://support.microsoft.com/en-us/windows/change-colors-in-windows-d26ef4d6-819a-581c-1581-493cfcc005fe)|1.0|
|`retro`|Use the current foreground terminal color|1.0|
|`rainbow`|Progress through a rainbow of colors|1.0|
|`sixel`|Use sixel images; requires a terminal that supports displaying sixels, such as [Windows Terminal](https://github.com/microsoft/terminal/releases) 1.22.2362 or later|1.9|
|`disabled`|No progress will be displayed|1.9|

```json
    "visual": {
        "progressBar": "accent"
    },
```

### anonymizeDisplayedPaths

Replaces some known folder paths with their respective environment variable. Defaults to true.

```json
    "visual": {
        "anonymizeDisplayedPaths": true
    },
```

### enableSixels

Enables output of sixel images in certain contexts. Defaults to false.

```json
    "visual": {
        "enableSixels": true
    },
```

## Install Behavior

The `installBehavior` settings affect the default behavior of installing and upgrading (where applicable) packages.

### Disable Install Notes

The `disableInstallNotes` behavior affects whether installation notes are shown after a successful install. Defaults to `false` if value is not set or is invalid.

```json
    "installBehavior": {
        "disableInstallNotes": true
    },
```

### Portable Package User Root

The `portablePackageUserRoot` setting affects the default root directory where packages are installed to under `User` scope. This setting only applies to packages with the `portable` installer type. Defaults to `%LOCALAPPDATA%/Microsoft/WinGet/Packages/` if value is not set or is invalid.

> Note: This setting value must be an absolute path.

```json
    "installBehavior": {
        "portablePackageUserRoot": "C:/Users/FooBar/Packages"
    },
```

### Portable Package Machine Root

The `portablePackageMachineRoot` setting affects the default root directory where packages are installed to under `Machine` scope. This setting only applies to packages with the `portable` installer type. Defaults to `%PROGRAMFILES%/WinGet/Packages/` if value is not set or is invalid.

> Note: This setting value must be an absolute path.

```json
    "installBehavior": {
        "portablePackageMachineRoot": "C:/Program Files/Packages/Portable"
    },
```

### Skip Dependencies

The 'skipDependencies' behavior affects whether dependencies are installed for a given package. Defaults to 'false' if value is not set or is invalid.

```json
    "installBehavior": {
        "skipDependencies": true
    },
```

### Archive Extraction Method

The `archiveExtractionMethod` behavior affects how installer archives are extracted. Currently there are two supported values: `Tar` or `ShellApi`.
`Tar` indicates that the archive should be extracted using the tar executable ('tar.exe') while `shellApi` indicates using the Windows Shell API. Defaults to `shellApi` if value is not set or is invalid.

```json
    "installBehavior": {
        "archiveExtractionMethod": "tar" | "shellApi"
    },
```

### Preferences and Requirements

Some of the settings are duplicated under `preferences` and `requirements`. `preferences` affect how the various available options are sorted when choosing the one to act on.  For instance, the default scope of package installs is for the current user, but if that is not an option then a machine level installer will be chosen. `requirements` filter the options, potentially resulting in an empty list and a failure to install. In the previous example, a user scope requirement would result in no applicable installers and an error.

Any arguments passed on the command line will effectively override the matching `requirement` setting for the duration of that command.

> [!NOTE]
>
> - These settings are only applied for the `winget install` command.
> - Other commands like `winget configure` are not affected by these settings.

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

### Installer Types

The `installerTypes` behavior affects what installer types will be selected when installing a package. The matching parameter is `--installer-type`.

```json
    "installBehavior": {
        "preferences": {
            "installerTypes": ["msi", "msix"]
        }
    },
```

### Default install root

The `defaultInstallRoot` affects the install location when a package requires one. This can be overridden by the `--location` parameter. This setting is only used when a package manifest includes `InstallLocationRequired`, and the actual location is obtained by appending the package ID to the root.

```json
    "installBehavior": {
        "defaultInstallRoot": "C:/installRoot"
    },
```

### Maximum resumes

The `maxResumes` setting determines the maximum number of times that a command may be resumed automatically. The default value is 3.

```json
    "installBehavior": {
        "maxResumes": 3
    },
```

> Note: [The resume behavior is an experimental feature.](#resume)

## Uninstall Behavior

The `uninstallBehavior` settings affect the default behavior of uninstalling (where applicable) packages.

### Purge Portable Package

The `purgePortablePackage` behavior affects the default behavior for uninstalling a portable package. If set to `true`, uninstall will remove all files and directories relevant to the `portable` package. This setting only applies to packages with the `portable` installer type. Defaults to `false` if value is not set or is invalid.

```json
    "uninstallBehavior": {
        "purgePortablePackage": true
    },
```

## Configure Behavior

The `configureBehavior` settings affect the default behavior of applying a configuration.

### Default Module Root

The `defaultModuleRoot` behavior affects the default root directory where modules are installed to. Defaults to `%LOCALAPPDATA%/Microsoft/WinGet/Configuration/Modules` if value is not set or is invalid.

> Note: This setting value must be an absolute path.

```json
    "configureBehavior": {
        "defaultModuleRoot": "C:/Program Files/Modules/"
    },
```

## Telemetry

The `telemetry` settings control whether winget writes ETW events that may be sent to Microsoft on a default installation of Windows.

See [details on telemetry](../README.md#datatelemetry), and our [primary privacy statement](../PRIVACY.md).

### disable

```json
    "telemetry": {
        "disable": true
    },
```

If set to true, the `telemetry.disable` setting will prevent any event from being written by the program.

## Logging

The `logging` settings control the level of detail in log files.

### level

 `--verbose-logs` will override this setting and always creates a verbose log.
Defaults to `info` if value is not set or is invalid.

```json
    "logging": {
        "level": "verbose" | "info" | "warning" | "error" | "critical"
    },
```

### channels

The valid values in this array are defined in the function `GetChannelFromName` in the [logging code](../src/AppInstallerSharedLib/AppInstallerLogging.cpp).  These align with the ***channel identifier*** found in the log files.  For example, ***`CORE`*** in:

```plaintext
2023-12-06 19:17:07.988 [CORE] WinGet, version [1.7.0-preview], activity [{24A91EA8-46BE-47A1-B65C-CEBCE90B8675}]
```

In addition, there are special values that cover multiple channels.  `default` is the default set of channels, while `all` is all of the channels.  Invalid values are ignored.

```json
    "logging": {
        "channels": ["default"]
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

## Interactivity

The `interactivity` settings control whether winget may show interactive prompts during execution. Note that this refers only to prompts shown by winget itself and not to those shown by package installers.

### disable

```json
    "interactivity": {
        "disable": true
    },
```

If set to true, the `interactivity.disable` setting will prevent any interactive prompt from being shown.

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

### resume

This feature enables support for some commands to resume.
You can enable the feature as shown below.

```json
   "experimentalFeatures": {
       "resume": true
   },
```

### configuration03

This feature enables the configuration schema 0.3.
You can enable the feature as shown below.

```json
   "experimentalFeatures": {
       "configuration03": true
   },
```

### configureSelfElevate

This feature enables configure commands to request elevation as needed.
Currently, this means that properly attributed configuration units (and only those) will be run through an elevated process while the rest are run from the current context.

```json
   "experimentalFeatures": {
       "configureSelfElevate": true
   },
```

### configureExport

This feature enables exporting a configuration file.
You can enable the feature as shown below.

```json
   "experimentalFeatures": {
       "configureExport": true
   },
```
