## New in v1.29

# New Feature: Filter List and Search command by install scope

This adds the ability to filter the list returned by the `winget list` and `winget search` commands by the install scope
using the `--scope` parameter. Valid scope values are `user` and `machine`. If the parameter is not given, any install
scopes are permitted.

> [!TIP]
> You can combine parameters for the `--source` and `--scope` parameters when filtering.

# New Feature: Source Priority

> [!NOTE]
> Experimental under `sourcePriority`; defaulted to disabled.

With this feature, one can assign a numerical priority to sources when added or later through the `source edit`
command. Sources with higher priority are sorted first in the list of sources, which results in them getting put first
in the results if other things are equal.

> [!TIP]
> Search result ordering in winget is currently based on these values in this order:
> 1. Match quality (how well a valid field matches the search request)
> 2. Match field (which field was matched against the search request)
> 3. Source order (was always relevant, but with priority you can more easily affect this)

Beyond the ability to slightly affect the result ordering, commands that primarily target available packages
(largely `install`) will now prefer to use a single result from a source with higher priority rather than prompting for
disambiguation from the user. Said another way, if multiple sources return results but only one of those sources has
the highest priority value (and it returned only one result) then that package will be used rather than giving a
"multiple packages were found" error. This has been applied to both winget CLI and PowerShell module commands.

### REST result match criteria update

Along with the source priority change, the results from REST sources (like `msstore`) now attempt to correctly set the
match criteria that factor into the result ordering. This will prevent them from being sorted to the top automatically.

## Bug Fixes

> 1. Prior to this version, it was permitted to provide a `--source` filter for the `winget list` command, however the parameter was ignored. Now if a filter is provided packages will be check before being returned from the `winget list` command.
