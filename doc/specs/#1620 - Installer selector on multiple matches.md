---
author: Darshan Rander @SirusCodes
created on: 2022-10-29
last updated: 2022-11-11
issue id: 1620
---

# Spec Title

For [#1620](https://github.com/microsoft/winget-cli/issues/1620)

## Abstract

Currently, the winget.exe client simply lists all the matches and if the user wants to install the app they need to re-run the command with Id.

The UX can be improved here by giving users an option to select the package instead of asking them to re-run the command with Id.

This can be extended to `upgrade`, `show` and `uninstall` commands.

## Inspiration

Whenever I use Winget to install any application there are high chances that there are name collisions with other names and I get multiple suggestions in the table and it's annoying to copy the ID and rerun the command. I guess this is the case for a lot of others as well.

## Solution Design

If we find multiple packages then the user should get a selector.

> It should not be shown if `--diable-interactivity` is set.

### Settings

The user needs to **opt-in** to this feature.

Winget can use `--prompt` and `--no-prompt` flags to enable and disable it respectively.

The user can change the default setting to always show the prompt by -

```pwsh
"packageDisambiguation": "prompt"
```

and disable it by -

```pwsh
"packageDisambiguation": "no-prompt"
```

### Valid inputs

- Number - It should match with a _row number_ (0 will exit the selector)
- Text - It should match either the package _Name_ or _Id_
- `--source` flag - It refines the search with a specific package source. It can only be used if a user enters text or standalone.

### Invalid inputs

If a user enters an invalid value it should show an error asking the user for input again,

Example: `1ab`

```pwsh
Invalid input, please enter row number, or package name or id.
```

If a user enters a number that is out of the shown list then they get an error to enter numbers that are in the list.

Example: `23` in the list of `20`` entries

```pwsh
Please enter row number which are present in entries above.
```

### Number of results

We should add a `--list-count` flag to customize the number of packages listed in the prompt. By default, the user will see a list of 20 packages.

If the number of packages is more than allowed it should show a text saying how many more packages are available

```pwsh
...
 20  Example Example.Example20 winget

There are 6 more packages available.
Please select a row number or refine the search by package name or ID.
>
```

If a user enters the package name or ID then we should scan the whole list including the ones which are not shown to the user.

If they enter a number then it should be in the range of the list shown to the user else it should be considered invalid input.

### Refine the search

A user can search results by entering text input. The text input should be compared with the Name and ID in the result.

If a user enters a Name or ID and we found only an exact match then start installing the package else we should show the new refined list to the user.

The text input is said to be invalid only if it doesn't match any of the names or IDs. The user will see an error for invalid input and will be asked to retry.

If the user enters the name/ID and source or only source and there isn't any source then the user will see an error message which says that cannot find the package with the source.

## UI/UX Design

This is how the prompt should look like...

```pwsh
> winget install Example
Multiple packages found matching.
No. Name       Id                   Source
-------------------------------------------
 1  Example    Example.Example1     msstore
 2  Example    Example.Example2     winget
 ...
 20 Example20  Example.Example20    msstore
There are 8 more packages.

Please select a row number, or enter package name or ID to refine search...
> _
```

If a user enters a row number between 1-20 then install the package

```pwsh
Please select a row number, or enter package name or ID to refine search...
> 2
Downloading Example (Example.Example2)...
```

If a user enters an invalid input.

```pwsh
Please select a row number, or enter package name or ID to refine search...
> 23
Please enter row number which are present in entries above.
>
```

If the user enters a name or ID then we should refine the list.

```pwsh
...
Please select a row number, or enter package name or ID to refine search...
> Example

Refined the search to Name/ID with `Example` in it.
No. Name       Id                   Source
-------------------------------------------
 1  Example    Example.Example1     msstore
 2  Example    Example.Example2     winget
 3  Example    Example.Example22    msstore

Please select a row number, or enter package name or ID to refine search...
> _
```

A user can use `--source` flag

Now we only have a package with ID `Example.Example2` in the list because of the queries the user wrote.

```pwsh
Refined the search to Name/ID with `Example` in it.
No. Name       Id                   Source
-------------------------------------------
 1  Example    Example.Example1     msstore
 2  Example    Example.Example2     winget
 3  Example    Example.Example22    msstore

Please select a row number, or enter package name or ID to refine search...
> --source winget
Downloading Example (Example.Example2)...
```

If the `--source` query matches nothing then ask the user to retry.

```pwsh
No. Name       Id                   Source
-------------------------------------------
 1  Example    Example.Example1     msstore
 2  Example2   Example.Example2     winget
...
 20 Example20  Example.Example20    msstore
There are 8 more packages.

Please select a row number, or enter package name or ID to refine search...
> Example --source winget
Cannot find 'Example' in 'winget', please try again.
> _
```

## Capabilities

### Accessibility

The screen reader read the text properly as it reads the current implementation.

### Security

It should not have any security issues.

### Reliability

The user might enter the number incorrectly in rush and might need to kill the running installation process and restart it.

### Compatibility

This won't work in MinQTTY. It's a [known issue](https://github.com/git-for-windows/build-extra/blob/main/ReleaseNotes.md?rgh-link-date=2022-10-21T03%3A13%3A26Z#known-issues) in Git-for-windows.

### Performance, Power, and Efficiency

## Potential Issues

It should not be shown if `--diable-interactivity` is set.

There should be no effect of the `--silent` flag on the selector.

## Future considerations

Currently, Winget does not support installing multiple packages in a single command.

For Example -

```pswh
winget install Powershell Git
```

The above command will not install anything.

If this is ever supported, potential considerations include having _multiple_, _range_ and _not_ selectors.

- multiple selector (1 3): A **space** separated number should install packages of that number.
- range selector (1-3): A **hyphen** separated number should install all the packages including the start and end numbers.
- not selector (^2): A number with a **carat** symbol should install all packages except the number marked with a carat symbol.

## Resources

- [winget#301 (comment)](https://github.com/microsoft/winget-cli/issues/301#issuecomment-940172239) a snapshot from the [yay](https://github.com/Jguer/yay).
