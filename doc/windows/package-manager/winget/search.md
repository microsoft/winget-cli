---
title: search Command
description: Queries the sources for available applications that can be installed
ms.date: 04/28/2020
ms.topic: overview
ms.localizationpriority: medium
---

# search command (winget)

The **search** command of the [winget](index.md) tool queries the sources for available applications that can be installed.

The **search** command can show all applications available, or it can be filtered down to a specific application. The **search** command is used typically to identify the string to use to install a specific application.

## Usage

`winget search [[-q] <query>] [<options>]`

The following command aliases are available: \
`find`

![Screenshot of the Windows Power Shell window displaying the results of the winget search.](images/search.png)

## Arguments

The following arguments are available.

| Argument  | Description |
 --------------|-------------|
| **-q, --query** |  The query used to search for an app. |

## Show all

If the search command includes no filters or options, it will display all available applications in the default source. You can also search for all applications in another source if you pass in just the **source** option.

## Search strings

Search strings can be filtered with the following options.

| Option  | Description |
 --------------|-------------|
| **--id** | Limits the search to the ID of the application. The ID includes the publisher and the application name. |
| **--name** | Limits the search to the name of the application. |
| **--moniker** | Limits the search to the moniker specified. |
| **--tag** | Limits the search to the tags listed for the application. |
| **--cmd, --command** | Limits the search to the commands listed for the application. |
| **-s, --source** | Find package using the specified source. |
| **-n, --count** | Show no more than specified number of results (between 1 and 1000). |
| **-e, --exact** | Find package using exact match. |
| **--header** | Optional Windows-Package-Manager REST source HTTP header. |
| **--accept-source-agreements** | Accept all source agreements during source operations. |
| **--versions** | Show available versions of the package. |
| **--sort** | Sort package results by a field; repeat to add tie-breakers. |
| **--ascending, --asc** | Sort package results in ascending order. |
| **--descending, --desc** | Sort package results in descending order. |
| **-?, --help** | Gets additional help on this command. |
| **--wait** | Prompts the user to press any key before exiting. |
| **--logs, --open-logs** | Open the default logs location. |
| **--verbose, --verbose-logs** | Enables verbose logging for winget. |
| **--disable-interactivity** | Disable interactive prompts. |

The string will be treated as a substring. The search by default is also case-insensitive. For example, `winget search micro` could return the following:

* Microsoft
* Microscope
* MyMicro

## Search options

The search commands supports a number of options or filters to help limit the results.

| Option  | Description |
 --------------|-------------|
| **-e, --exact** | Uses the exact string in the query, including checking for case-sensitivity. It will not use the default behavior of a substring. |
| **-n, --count** | Restricts the output of the display to the specified count. |
| **-s, --source** | Restricts the search to the specified [source](source.md) name. |

## Result ordering

Results are displayed by relevance, then by name within equal-relevance groups, with or without a query. Relevance compares match type first, then matched-field importance. Name sorting is case-insensitive and uses Windows numeric sorting, so otherwise matching names containing `9.0` precede `10.0`.

Use `--sort` repeatedly to select fields in priority order, for example `winget search Runtime --sort relevance --sort name --descending`. Sort fields override `output.sortOrder`; `--ascending` / `--descending` override `output.sortDirection`. These [settings](list.md#sort-via-user-settings) override the presentation defaults when the corresponding arguments are absent. For example, `["relevance"]` retains incoming order within relevance groups, while `["name"]` makes name the primary key. Fields without a corresponding column do not distinguish rows.

Result limits are applied before presentation sorting. This changes neither repository/API result ordering nor the version ordering shown by `--versions`. Multiple-match candidate tables, including interactive package selection, use the same presentation policy.

## Related topics

* [Use the winget tool to install and manage applications](index.md)
