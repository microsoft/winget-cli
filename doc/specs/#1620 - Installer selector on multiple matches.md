---
author: Darshan Rander @SirusCodes
created on: 2022-10-29
last updated: 2022-10-29
issue id: 1620
---

# Spec Title

For [#1620](https://github.com/microsoft/winget-cli/issues/1620)"

## Abstract

The winget.exe client simply lists all the matches and if the user wants to install the app they need to re-run the command with Id.

The UX can be improved here by giving users an option to select the package instead of asking them to re-run the command with Id.

## Inspiration

It is not a good experience to copy Id and then run the command again.

## Solution Design

If we find multiple packages then the user should get a selector.

winget should ask for user input, for example -

```pwsh
> winget install Powershell
Multiple packages found matching.
No. Name       Id                   Source
-------------------------------------------
 1  PowerShell 9MZ1SNWT0N5D         msstore
 2  PowerShell Microsoft.PowerShell winget

Please select the packages (eg. 2): <user-input>

Installing <user-input>...
```

> `<user-input>` - is the input by user

After the user will give the input the installation should start.

## UI/UX Design

The idea is explained in [Solution Design](#solution-design).

## Capabilities

### Accessibility

It should not have any direct impact on screen readers.

### Security

It should not have any security issues.

### Reliability

This will help the users to have a better experience and would decrease the chance of human error while copy-pasting the Id.

### Compatibility

It will not break anything.

### Performance, Power, and Efficiency

## Potential Issues

The user might not be negatively impacted by this, as it is geared towards simplifying the installation process in case of multiple matches.

## Future considerations

As far as I know currently (v1.3.2691) widget doesn't support installing multiple packages in a single command.

For Example -

```pswh
winget install Powershell Git
```

The above command will not install anything.

If will ever support this then we might consider having _multiple_, _range_ and _not_ selectors.

- multiple selector (1 3): A **space** separated number should install packages of that number.
- range selector (1-3): A **hyphen** separated number should install all the packages including the start and end numbers.
- not selector (^2): A number with a **carat** symbol should install all packages except the number marked with a carat symbol.

## Resources

- [winget#301 (comment)](https://github.com/microsoft/winget-cli/issues/301#issuecomment-940172239) a snapshot from the [yay](https://github.com/Jguer/yay).
