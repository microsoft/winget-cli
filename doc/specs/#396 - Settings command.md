---
author: Ruben Guerrero @msftrubengu
created on: 2020-06-03
last updated: 2021-07-14
issue id: 396
---

# Settings command

For [#396](https://github.com/microsoft/winget-cli/issues/396)

## Abstract

The winget.exe client must support having a feature that allows users to specify settings. This will be in the form of an editable file. The file must be in a location where a user can easily access it and modify it.

## Inspiration

Add ability for user to set their own preferences.

## Solution Design

### Format

The WinGet settings file needs to be in a readable format for users. We considered using other options, like the registry, but having a file makes it more accessible to users.

#### Option 1: YAML 

WinGet already knows how to handle YAML files via yaml-cpp. To follow the manifest style, the properties will be PascalCased.

#### Option 2: JSON

JSON is a popular format that most applications tend to use. It is also possible to parse JSON with yaml-cpp as YAML is a superset of JSON, but a proper JSON parser might be needed depending on how well yaml-cpp handles it. Options are third party JSON parsers or Microsoft cpp winrt implementation `winrt::Windows::Data::Json::JsonObject`. Properties will be camelCased.

There is also the concern about comments because JSON doesn't support them. There are different approaches we could take.
- Go with JSON standard and support properties named `__comments` which will be ignored when parsing the file.
- Use jsoncpp and allow C type comments.
- Use yaml-cpp and allow YAML comments.

Based on this information we are going to use JSON and jsoncpp as a parser. For the comments issue, we are having a discussion here [#416](https://github.com/microsoft/winget-cli/issues/416)

### Location

WinGet can either run in package context or not. That means the location of the settings file will be determined depending on the context.

Package Context: %LOCALAPPDATA%\Packages\Microsoft.DesktopAppInstaller_8wekyb3d8bbwe\LocalState\settings.json. For more about UWP file system see [this](https://docs.microsoft.com/windows/uwp/get-started/fileio-learning-track#access-the-file-system) 

Non-package context: %LOCALAPPDATA%\Microsoft\Winget\settings.json

### Command

A WinGet command will be added to support settings.

Options:
- settings (preferred)
- config
- configure

A command is a better option than having an argument, for example `winget --settings`, because it let us add more commands into it in the future such as set and unset.

The expectation is that when the user enters `winget settings` the settings file will be opened in the user's default text editor via ShellExecute. If the user doesn't have any file type association with `.json`, the default will be to open it with notepad.exe 

There will also be a telemetry point added into the command as the other commands have.

### Backup Settings File

A user might make a mistake that could make the settings file unparsable. To protect against this there must be a backup settings file with the latest known good settings file. The flow is the following:

1. `winget settings`
2. If settings file can be parsed copy to settings.json.backup
3. Open settings file
4. Wait for next command
5. If settings file can be parsed use it; otherwise, use settings.json.backup and warn user.

This mechanism allows the settings file to be resilient against mistakes.

### Creating file

A settings file will only be created if the user runs `winget settings` and one of the following occurs:

1. First time use: Files settings.json or settings.json.backup don't exist. Winget will create both settings.json and settings.json.backup files using the default settings text. Winget will open settings.json in an editor.

2. settings.json deleted: If the settings file doesn't exist but settings.json.backup exists, the settings file will be created using settings.json.backup. Winget will open settings.json in an editor. If the user intended to remove their settings the recommendation should be an empty JSON file, not deleting the file.

3. settings.json and settings.json.backup deleted: Scenario is identical to 1.

### Loading settings

Since the settings file doesn't exist at this time, we can't force the creation of it at WinGet install time. Moreover, the file will only be created at `winget settings` time, so any other command executed before it must work as it does right now. This means that winget must work without the existence of the file, which are the default settings.

These leave us with three different sources of settings in order of importance:
1. settings.json
2. settings.json.backup
3. Default settings 

Setting will be loaded as following: 

```
if settings exists and valid
    load settings
else if backup exists and valid
    load settings backup
else
    use default settings
```

Where valid means that syntax and semantic checks pass. For now, semantics checks will be part of the validation. If one setting is semantically incorrect and we fallback to backup proves to be annoying to users, checks can be relaxed so that only syntax failures are fatal and semantic errors are warnings. We could also in the future add a `winget settings validate` to improve the experience. 

We cannot force the user to upgrade, so it is possible for someone to add a setting for a future version that is not supported. There is not an easy way to detect it which means that loading the settings will warn of an unknown property. The user will need to verify the documentation and the version of winget that is running via `winget --info`. 

#### Errors and Warnings

Loading settings will never fail, but warnings might happen. In addition to warnings regarding syntax and semantic validation, a warning will be printed if settings.json.backup is being used. If both settings and backup files failed to load and they exist another warning will be printed. There is no warning if the files don't exist. All these warnings must be localized as other text used in the project.

- Settings Warning: Settings file failed loading. Using backup file.
- Settings Backup Warning: Settings backup failed loading. Using default settings.

### Documentation

All settings must be documented in the winget-cli repository in doc\Settings.md. The settings file will need to have a link to this file or some other Microsoft documentation site for reference.

### Version property

A version property can be added to the settings, such as the manifest has, to have a more structured validation. I am currently opposed to the idea because, for future settings, we will need to always bump up the version and force the users to modify two pieces: the settings they want to use and the version property. I can also see settings becoming more dynamic and bumping the version per addition/removal seems like an overkill.

### Settings

Two settings will be implemented for the sake of the discussion and to not provide a setting feature without actual settings.

### Progress Bar Visual Style

This setting will specify the color of the progress bar that WinGet displays. It will only support the current visual style that WinGet already supports and no new ones will be added for now.

Possible values:
1. accent (default)
2. retro
3. rainbow

```
  "Visual": {
      "ProgressBar": "accent"
    }
```

#### Override

The user must be able to keep using the preferred visual style via arguments as it is now. This has more priority than the style defined in the settings file, because is saying it want that style for the current run.

#### Special request

The author of this document firmly believes that `--plain` must be replaced with `--retro`.

### Source auto update

Currently, WinGet updates the source after 5 minutes. This setting will will enable users to set the timeout in minutes.

Value must be integers with a minimum of 0. An arbitrary limit limit can be set but is not strictly necessary. A value of 0 indicates no update.

```
  "Source": {
      "AutoUpdateIntervalInMinutes": 5
    }
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

This feature allows the ability to expand the customization of winget for any user. It is also designed to be expanded with future settings commands that don't fit the scope of this feature. For example, one could use `winget settings set SOME_SETTING on` to modify the settings without the need of editing a file.

## References

@JohnMcPMS for telling me what to type.