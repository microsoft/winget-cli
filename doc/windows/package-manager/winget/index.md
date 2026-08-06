---
title: Use the winget tool to install and manage applications
description: The winget command line tool enables developers to discover, install, upgrade, remove, repair, download, configure, and manage applications on Windows computers.
ms.date: 2026-08-06
ms.topic: overview
ms.localizationpriority: medium
---

# Use the winget tool to install and manage applications

The **winget** command line tool enables users to discover, install, upgrade, remove, repair, download, pin, configure, and manage applications on Windows 10 and Windows 11 computers. This tool is the client interface to the Windows Package Manager service.

## Install winget

There are several ways to install the **winget** tool:

* The **winget** tool is included in the flight or preview version of [Windows App Installer](https://apps.microsoft.com/detail/9nblggh4nns1?ocid=9nblggh4nns1_ORSEARCH_Bing&rtc=1&activetab=pivot:overviewtab). You must install the preview version of **App Installer** to use **winget**. To gain early access, submit your request to the [Windows Package Manager Insiders Program](https://aka.ms/AppInstaller_InsiderProgram). Participating in the flight ring will guarantee you see the latest preview updates.

* Participate in the [Windows Insider Dev Channel](https://insider.windows.com/understand-flighting).

* Install the Windows Desktop App Installer package located on the [Releases page for the winget repository](https://github.com/microsoft/winget-cli/releases).

> The **winget** tool is supported on Windows 10, version 1809 (build 17763) and above.

## Use winget

After **App Installer** is installed, you can run **winget** by typing `winget` from a Command Prompt.

One of the most common usage scenarios is to search for and install a favorite tool.

1. To [search](search.md) for a tool, type `winget search <appname>`.
2. After you have confirmed that the tool you want is available, you can [install](install.md) the tool by typing `winget install <appname>`.
3. To get a complete list of commands, type `winget --help`.

### Commands

The current preview of the **winget** tool supports the following commands.

| Command | Description |
|---------|-------------|
| [install](install.md) | Installs the selected package. |
| [show](show.md) | Shows information about a package. |
| [source](source.md) | Manage sources of packages. |
| [search](search.md) | Find and show basic info of packages. |
| [list](list.md) | Display installed packages. |
| [upgrade](upgrade.md) | Shows and performs available upgrades. |
| [uninstall](uninstall.md) | Uninstalls the selected package. |
| [hash](hash.md) | Helper to hash installer files. |
| [validate](validate.md) | Validates a manifest file. |
| [settings](settings.md) | Open settings or set administrator settings. |
| [features](features.md) | Shows the status of experimental features. |
| [export](export.md) | Exports a list of the installed packages. |
| [import](import.md) | Installs all the packages in a file. |
| [pin](pin.md) | Manage package pins. |
| [configure](configure.md) | Configures the system into a desired state. |
| [download](download.md) | Downloads the installer from a given package. |
| [repair](repair.md) | Repairs the selected package. |
| [dscv3](dscv3.md) | DSC v3 resource commands. |
| [mcp](mcp.md) | MCP information. |

### Options

The current version of the **winget** tool supports the following global options.

| Option | Description |
|--------------|-------------|
| **-v, --version** | Display the version of the tool. |
| **--info** | Display general info of the tool. |
| **-?, --help** | Shows help about the selected command. |
| **--wait** | Prompts the user to press any key before exiting. |
| **--logs, --open-logs** | Open the default logs location. |
| **--verbose, --verbose-logs** | Enables verbose logging for winget. |
| **--nowarn, --ignore-warnings** | Suppresses warning outputs. |
| **--disable-interactivity** | Disable interactive prompts. |
| **--proxy** | Set a proxy to use for this execution. |
| **--no-proxy** | Disable the use of proxy for this execution. |

## Supported installer formats

The current version of the **winget** tool supports the following types of installers:

* EXE
* INNO
* NULLSOFT
* MSI
* APPX
* MSIX
* BURN
* PORTABLE
* ZIP

## Scripting winget

You can author batch scripts and PowerShell scripts to install multiple applications.

```CMD
@echo off
Echo Install Powertoys and Terminal
REM Powertoys
winget install Microsoft.Powertoys
if %ERRORLEVEL% EQU 0 Echo Powertoys installed successfully.
REM Terminal
winget install Microsoft.WindowsTerminal
if %ERRORLEVEL% EQU 0 Echo Terminal installed successfully. %ERRORLEVEL%
```

## Related topics

* [help command](help.md)
* [WinGet return codes](returnCodes.md)
