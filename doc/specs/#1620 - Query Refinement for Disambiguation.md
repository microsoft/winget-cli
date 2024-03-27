---
author: Darshan Rander @SirusCodes and Kaleb Luedtke @Trenly
created on: 2022-10-29
last updated: 2022-11-28
issue id: 1620
---

# Query Refinement for Disambiguation

For [#1620](https://github.com/microsoft/winget-cli/issues/1620)

## Abstract

When the user runs the install command and the winget client cannot disambiguate the query down to a single package, the client will list potential matches. The user then needs to re-run the install command with the package identifier to install the package.

The UX can be improved by giving users an option to select the package instead of forcing the command to be re-run with the package identifier.
This selection option can also be extended to the `upgrade`, `show` and `uninstall` commands.

## Inspiration

There are many similarly-named packages in winget. When users try to install any application, there is a high chance of a collision causing multiple suggestions in the returned table. It is an annoyance to copy the package identifier and re-run the command.

## Solution Design

When the query for `install`, `upgrade`, `show`, or `uninstall` cannot be disambiguated to a single package, the user should be given the opportunity to continuously refine their query.

> Query refinement should not occur if `--diable-interactivity` is used or if interactivity has been disabled through settings. Query refinement is not required for COM callers.

### Settings

This shall be an **opt-in** only feature. The setting for this feature shall be placed uder the `interactivity` object. 

In order to enable the query refinement feature -
> Note: The setting could also be called `packageDisambiguation`
```json
"interactivity" : {
   "refineAmbiguousQueries": true
}
```
In order to disable the query refinement feature -
```json
"interactivity" : {
   "refineAmbiguousQueries": false // This is the default behavior
}
```

From the command line, users should be able to use `--refine-query` to override the user setting and show the query refinement, and `--no-refinement` to override the user setting and not show the query refinement.

### Refinement Source
The results from the query needing refinement should be placed into a temporary database to be used for further refinement. This database should be treated as a new temporary source to be queried. Upon completion of the command, this temporary database shall be removed. If the database is already existing and a new command is run, the database shall be overwritten.

### Refinement queries
Input to the package refinement should be treated as a query by default. This should match the behavior of the parent command. Each entry should also be assigned an index number which can be used to reference the package. The index number shall be treated as a fully qualified reference, and entry of an exact match to the number as a query shall take precedent.

> Example: A user runs `winget install Example` and receives the result:
> ```
> Multiple packages found matching input criteria.
> Number Name       Id                   Source
> -----------------------------------------------
>   1    Example 3  Example.Example3     msstore
>   2    Example 2  Example.Example2     winget
> ```
> If the user were to input `'Example 2'`, it would be treated as `winget install -q 'Example 2'` using only the results in the table above as the source
 
Users should be able to refine their query using any of the standard or shared flags. 
This includes, but is not limited to -
- `--id` - Filter results by id
- `--name` - Filter results by name
- `--moniker` - Filter results by the moniker
- `--tag` - Filter results by tag
- `--cmd` or `--command` - Filter results by command
- `-s` or `--source` - Find the package using the specified source
- `-e` or `--exact` - Find the package using exact match

### Number of results

The `-n` or `--number` flag should be available for the `install`, `upgrade`, `show`, and `uninstall` commands to limit the number of packages returned for refinement.

If the number of packages is more than allowed it should show the message that additional entries have been truncated due to the result limit. Packages truncated from the results should be included in the database for refinement queries.

### Tab Completion
Tab completion shall complete the package identifier just as it would for the parent command of the refinement. The behavior of forward cycling with <kbd>Tab</kbd> and reverse cycling with <kbd>Shift</kbd>+<kbd>Tab</kbd> shall be retained.

### Invalid Entries
Since all entries are treated as a refinement, no entry is considered to be invalid. If a refinement query returns no results from the temporary database source, the user shall be shown the message that no packages match the input criteria.

## UI/UX Design

The table output to the user shall include, at minimum, the index number, package name, package identifier, and the source of the package

```pwsh
> winget install Example
Multiple packages found matching input criteria.
 Number Name      Id                Source
-------------------------------------------
   1    ExampleA  Example.Example1 msstore
   2    ExampleA  Example.Example2 winget
   ...
   27   Example27 Example.Example27 msstore

Please enter a row number, package name, ID, or query to refine search...
> _
```

If a user enters a number exactly matching the index number then install the package

```pwsh
Please enter a row number, package name, ID, or query to refine search...
> 2
Found ExampleA [Example.Example2] Version 1.0.0
This application is licensed to you by its owner.
...
```

If the user enters a name or ID then the query should be refined. If the package is not fully disambiguated, the refinement should continue using the refined results as the new source.

```pwsh
...
Please enter a row number, package name, ID, or query to refine search...
> ExampleA

Refined the search to Name/ID with `Example` in it.
Number Name       Id                   Source
-------------------------------------------
   1   ExampleA    Example.Example1     msstore
   2   ExampleA    Example.Example2     winget
   3   ExampleA    Example.Example22    msstore

Please enter a row number, package name, ID, or query to refine search...
> _
```

If the user enters a query, the flags should be honored.
```pwsh
# Refined the search to Name/ID with `ExampleA` in it.
Number Name       Id                   Source
-------------------------------------------
   1   ExampleA    Example.Example1     msstore
   2   ExampleA    Example.Example2     winget
   3   ExampleA    Example.Example22    msstore

Please enter a row number, package name, ID, or query to refine search...
> --source winget
Found ExampleA [Example.Example2] Version 1.0.0
This application is licensed to you by its owner.
...
```

If a query matches nothing then exit the CLI with the message that the query couldn't find any result.

```pwsh
Number Name       Id                   Source
-------------------------------------------
   1   Example    Example.Example1     msstore
   2   Example2   Example.Example2     winget
...
   20  Example20  Example.Example20    msstore

Please enter a row number, package name, ID, or query to refine search...
> Example.Example1 --source winget
No package found matching input criteria
```

## Capabilities

### Telemetry
Telemetry data should be sent around the following items
* When a disambiguation list is shown
* What query caused the disambiguation to be shown
  
The current telemetry will need to be adjusted to reflect the following case
* If a user was able to disambiguate the package, the command result should be treated as a success

### Accessibility

The screen reader read the text properly as it reads the current implementation.

### Security

It should not have any security issues.

### Reliability

The user might enter the number incorrectly in rush and might need to kill the running installation process and restart it.

### Compatibility

This won't work in MinTTY. It's a [known issue](https://github.com/git-for-windows/build-extra/blob/main/ReleaseNotes.md?rgh-link-date=2022-10-21T03%3A13%3A26Z#known-issues) in Git-for-windows.

### Performance, Power, and Efficiency

## Potential Issues
* There should be no effect of the `--silent` flag on the selector.
* Having the `upgrade` command included in this would conflict with [#2627](https://github.com/microsoft/winget-cli/issues/2627)
  
  For example - `winget upgrade Microsoft` might have a match with hundreds of packages and use might not want to see this prompt. Although this is an opt-in feature, there might be a clash if both features are enabled.

## Future considerations
* Keyboard interactivity using <kbd>↑</kbd>, <kbd>↓</kbd>, and <kbd>Ener</kbd>

* Currently, Winget does not support installing multiple packages in a single command.

   For Example -

   ```pswh
   winget install Powershell Git
   ```

   The above command will not install anything.

   If this is ever supported, potential considerations include having _multiple_, _range_ and _not_ selectors.

   - multiple selector (1 3): A **space** separated number should install packages of that number.
   - range selector (1-3): A **hyphen** separated number should install all the packages including the start and end numbers.
   - not selector (^2): A number with a **carat** symbol should install all packages except the number marked with a carat symbol.
* Protection against mistyping the numeric identifier

## Resources

- [winget#301 (comment)](https://github.com/microsoft/winget-cli/issues/301#issuecomment-940172239) a snapshot from [yay](https://github.com/Jguer/yay).
