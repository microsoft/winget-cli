## New in v1.29

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

Beyond the ability to minorly affect the result ordering, commands that primarily target available packages
(largely `install`) will now prefer to use a single result from a source with higher priority rather than prompting for
disambiguation from the user. Said another way, if multiple sources return results but only one of those sources has
the highest priority value (and it returned only one result) then that package will be used rather than giving a
"multiple packages were found" error. This has been applied to both winget CLI and PowerShell module commands.

### REST result match criteria update

Along with the source priority change, the results from REST sources (like `msstore`) now attempt to correctly set the
match criteria that factor into the result ordering. This will prevent them from being sorted to the top automatically.

## Bug Fixes
