---
author: Ruben Guerrero @msftrubengu
created on: 2020-06-03
last updated: 2020-06-03
issue id: 396
---

# Settings command

For [#396](https://github.com/microsoft/winget-cli/issues/396)

## Abstract

The winget.exe client must support having a feature that allows users to specify settings. This will be in the form of an editable file. The file must be in a location where a user can easily get it and modify it.

## Inspiration

Add ability for user to set their own preferences.

## Solution Design

### Format

The WinGet settings file needs to be in a readable format for users.

#### Option 1: YAML 

WinGet already knows how to handle YAML files via yaml-cpp. To follow the manifest style, the properties will be PascalCased.

#### Option 2: JSON

JSON is the popular formats that most applications tend to use. It is also possible to parse JSON with yaml-cpp as YAML is a superset of JSON, but a proper JSON parser might be needed depending on how well yaml-cpp handles it. Options are third party json parsers or Microsoft cpp winrt implementation `winrt::Windows::Data::Json::JsonObject`. Properties will be camelCased. An schema will need to be created as well.

### Location

WinGet can either run in package context or not. That means the location of the settings file will be determine depending on the context. 

Package Context: %LOCALAPPDATA%\Packages\Microsoft.DesktopAppInstaller_8wekyb3d8bbwe\LocalState\settings.yaml

Non-package context: %LOCALAPPDATA%\Microsoft\Winget\settings.yaml

### Command

A WinGet command will be added to support settings. Expectation is that when the user enters the command the settings file will be opened in the user's default text editor.

Options:
- winget settings
- winget config
- winget configure

A command is a better option than having an argument, for example `winget --settings`, because it let us add more commands into it in the future such as set and unset.

### Documentation

All setting must be documented in the winget-cli repository in doc\Settings.md. The settings file will need to have a link to this file or some other Microsoft documentation site for reference. However, having a comment on JSON wouldn't be ideal.

### Backup Settings File

A user might make a mistake could make the settings file unparsable. To protect against this there must be a backup settings file with the latest known good settings file. The flow is the following:

1. winget settings
2. If settings file can be parsed copy to settings.backup
3. Open settings file
4. Wait for next command
5. If settings file can be parsed use it otherwise use settings.backup and warn user.

This mechanism allows to be resilient for mistakes done the settings file.

### First time use

Since the settings file doesn't exist at this time, we can't force the creation of it at WinGet install time. This means that WinGet must still work without the existence of the file. The file will be created when the user runs the settings command it it doesn't exists. The new file will reference the settings documentation. 

### Settings

Two settings will be implemented for the sake of the discussion and to not provide a setting feature without actual settings.

### Progress Bar Visual Style

This setting will specify the color of the progress bar that WinGet displays. It will only support the current visual style that WinGet already support and no new ones will be added for now.

Possible values:
1. accent (default)
1. rainbow
1. plain

YAML
```
Visual:
  - ProgressBar: accent
```

JSON
```
  "Visual": [
    {
      "ProgressBar": "accent"
    }
  ]
```

#### Override

The user must be able to keep using the preferred visual style via arguments as it is now. This has more priority than the style defined in the settings file, because is saying it want that style for the current run.

#### Special request

The author of this document firmly believes that `--plain` must be replaced with `--retro`.

### Source auto update

Currently, WinGet updates the source after 5 minutes. This setting will will enable users to set the timeout in minutes.

Value must be integers with a minimum of 0. An arbitrary limit limit can be set but is not strictly necessary. A value of 0 indicates no update.

YAML
```
Source:
  - AutoUpdateIntervalInMinutes: 5
```

JSON
```
  "Source": [
    {
      "AutoUpdateIntervalInMinutes": 5
    }
  ]
```

Having the unit defined in the property makes it self documented and avoids the pain of opening the settings file to see the unit.

When the value is set to 0 it is the user responsibility to update the source if needed via `winget source update`

## UI/UX Design

Executing `winget settings` will open settings file in the default text editor the user has for the file extension.

## Capabilities

It will allow winget to set and define any new settings in the future.

### Accessibility

Any description or output text that is added by consequence of this feature will need to be localized. Moreover, this feature allows a mechanism to add accessibility settings in the future.

### Security

This should not introduce any _new_ security concerns.

### Reliability

This is not expected to impact reliability.

Settings will not be transferred if the user changes the context in which WinGet runs. That is, if the user has the Microsoft provided WinGet package and uninstalls it and runs a non-packaged WinGet, the settings are gone. This is by design.

### Compatibility

Previous versions winget will not be able to read a setting file. An update to winget is required.

### Performance, Power, and Efficiency

This introduce a read file for every command being run. However this shouldn't affect the performance of WinGet.

## Potential Issues

If the file isn't intuitive users will experience difficulties setting what they need.

## Future considerations

This feature allows the ability to expand the customization of winget for any user. It is also design to be expand with future settings commands that doesn't fit the scope of this future. For example, one could see `winget settings set SOME_SETTING on` to modify the settings without the need of editing a file.

## References

@JohnMcPMS for telling my what to type.

