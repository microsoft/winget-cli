---
author: Roy MacLachlan rdmaclachlan/roy.maclachlan@microsoft.com
created on: 2023-09-26
last updated: 2023-09-26
issue id: 3693
---

# Export and Import WinGet Settings

Link to issue: "For [#3693](https://github.com/microsoft/winget-cli/issues/3693)"

## Abstract

The Windows Package Manager client does not have an automatable approach for the application of user settings. Each time I setup a Windows Sandbox, or new Virtual Machine I need to add my unique sources (`winget source add -t "Microsoft.REST" -a "https://winget.com/api" -n "REST"`), then to enable the desired user experiences I must run `winget settings` select a text editor, then either manually type in my configuration settings or copy them from another device.


## Inspiration

I frequently use WinGet to setup and configure my Windows Sandbox environments. However each time I connect to my Windows Sandbox I need to manually re-apply my user settings. 

## Solution Design

The new user experience will expand on the existing WinGet Settings / WinGet Settings Export commands. No changes will be made to the existing user experience.

The new sub-commands will provide the ability to export the current WinGet CLI user settings, or Import a preconfigured set of settings stored in a file on the local machine.

## UI/UX Design

The new sub-commands will have a parameter input that supports providing a path to a local JSON file that will or does contain the Winget user settings.

```
WinGet Settings Export
    -p, --Path    The path to the Winget user settings JSON file to be created.
```

```
WinGet Settings Import
    -p, --Path    The path to the existing Winget user settings JSON file.
```

### WinGet Settings Export CLI

The following command provides the ability to export the current user settings to a JSON file on the local machine. `--Path` represents the path to a file where the settings will be exported to.

`WinGet Settings Export --Path C:\WinGet\Settings.json`

The file generated will contain the settings that can be used on another device to import the settings.

### WinGet Settings Import CLI

The following command provides the ability to import the user settings from a JSON file to the current user's WinGet CLI settings. `--path` is representative of the source file that contains the WinGet CLI user settings.

`WinGet Settings Import --Path C:\WinGet\Settings.json`

### WinGet Settings Import GPO

The following Group Policy object will enable enterprise customers with the ability to apply their desired winGet user settings to devices within their environment.

**Title:** Apply WinGet User Settings

**Description:** This policy will allow you the ability to apply a pre-configured set of user settings to the installed WinGet CLI.

If you enable this policy, you can provide the UNC path to a configuration JSON file that will be applied to the WinGet CLI user settings.

If you disable, or do not configure this policy, The WinGet user settings will not be enforced on the client device.

## Capabilities

### Accessibility

Users can use a pre-loaded configuration file, preventing the need to copy and paste, or manually type in the desired user experience.

### Security

Improve security as it will ensure a consistent experience can be applied.

### Reliability

Consistent user experience.

### Compatibility

N/A

### Performance, Power, and Efficiency

## Potential Issues

None.

## Future considerations

None.

## Resources

[comment]: # Be sure to add links to references, resources, footnotes, etc.
