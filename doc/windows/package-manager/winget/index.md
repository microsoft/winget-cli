---
title: Use the winget tool to install and manage applications
description: The winget command line tool enables developers to discover, install, upgrade, remove and configure applications on Windows 10 computers.
ms.date: 10/22/2020
ms.topic: overview
ms.localizationpriority: medium
---

# Use the winget tool to install and manage applications

[!INCLUDE [preview-note](../../includes/package-manager-preview.md)]

The **winget** command line tool enables developers to discover, install, upgrade, remove and configure applications on Windows 10 computers. This tool is the client interface to the Windows Package Manager service.

The **winget** tool is currently a preview, so not all planned functionality is available at this time.

## Install winget

There are several ways to install the **winget** tool:

* The **winget** tool is included in the flight or preview version of [Windows App Installer](https://www.microsoft.com/p/app-installer/9nblggh4nns1?ocid=9nblggh4nns1_ORSEARCH_Bing&rtc=1&activetab=pivot:overviewtab). You must install the preview version of **App Installer** to use **winget**. To gain early access, submit your request to the [Windows Package Manager Insiders Program](https://aka.ms/AppInstaller_InsiderProgram). Participating in the flight ring will guarantee you see the latest preview updates.

* Participate in the [Windows Insider flight ring](https://insider.windows.com).

* Install the Windows Desktop App Installer package located on the [Releases page for the winget repository](https://github.com/microsoft/winget-cli/releases).

> [!NOTE]
> The **winget** tool requires Windows 10, version 1709 (10.0.16299), or a later version of Windows 10.

## Administrator considerations

Installer behavior can be different depending on whether you are running **winget** with administrator privileges.

* When running **winget** without administrator privileges, some applications may [require elevation](https://docs.microsoft.com/windows/security/identity-protection/user-account-control/) to install. When the installer runs, Windows will prompt you to [elevate](https://docs.microsoft.com/windows/security/identity-protection/user-account-control). If you choose not to elevate, the application will fail to install.  

* When running **winget** in an Administrator Command Prompt, you will not see [elevation prompts](/windows/security/identity-protection/user-account-control/how-user-account-control-works) if the application requires it. Always use caution when running your command prompt as an administrator, and only install applications you trust.

## Use winget

After **App Installer** is installed, you can run **winget** by typing 'winget' from a Command Prompt.

One of the most common usage scenarios is to search for and install a favorite tool.

1. To [search](search.md) for a tool, type `winget search <appname>`.
2. After you have confirmed that the tool you want is available, you can [install](install.md) the tool by typing `winget install <appname>`. The **winget** tool will launch the installer and install the application on your PC.
    ![winget commandline](images/install.png)

3. In addition to install and search, **winget** provides a number of other commands that enable you to [show details](show.md) on applications, [change sources](source.md), and [validate packages](validate.md). To get a complete list of commands, type: `winget --help`.
    ![winget help](images/help.png)

### Commands

The current preview of the **winget** tool supports the following commands.

| Command | Description |
|---------|-------------|
| [export](export.md) | Exports a list of the installed packages. |
| [features](features.md) | Shows the status of experimental features. |
| [hash](hash.md) | Generates the SHA256 hash for the installer. |
| [import](import.md) | Installs all the packages in a file. |
| [install](install.md) | Installs the specified application. |
| [list](list.md) | Display installed packages. |
| [search](search.md) | Searches for an application. |
| [settings](settings.md) | Open settings. |
| [show](show.md) | Displays details for the specified application. |
| [source](source.md) | Adds, removes, and updates the Windows Package Manager repositories accessed by the **winget** tool. |
| [validate](validate.md) | Validates a manifest file for submission to the Windows Package Manager repository. |
| [uninstall](uninstall.md) | Uninstalls the given package. |
| [upgrade](upgrade.md) |  Upgrades the given package. | 

### Options

The current preview of the **winget** tool supports the following options.

| Option | Description |
|--------------|-------------|
| **-v, --version** | Returns the current version of winget. |
| **--info** |  Provides you with all detailed information on winget, including the links to the license, privacy statement, and configured group policies. |
| **-?, --help** |  Shows additional help for winget. |

## Supported installer formats

The current preview of the **winget** tool supports the following types of installers:

* EXE
* MSIX
* MSI

## Scripting winget

You can author batch scripts and PowerShell scripts to install multiple applications.

``` CMD
@echo off  
Echo Install Powertoys and Terminal  
REM Powertoys  
winget install Microsoft.Powertoys  
if %ERRORLEVEL% EQU 0 Echo Powertoys installed successfully.  
REM Terminal  
winget install Microsoft.WindowsTerminal  
if %ERRORLEVEL% EQU 0 Echo Terminal installed successfully.   %ERRORLEVEL%
```

> [!NOTE]
> When scripted, **winget** will launch the applications in the specified order. When an installer returns success or failure, **winget** will launch the next installer. If an installer launches another process, it is possible that it will return to **winget** prematurely. This will cause **winget** to install the next installer before the previous installer has completed.

## Missing tools

If the [community repository](../package/repository.md) does not include your tool or application, please submit a package to our [repository](https://github.com/microsoft/winget-pkgs). By adding your favorite tool, it will be available to you and everyone else.

## Customize winget settings

You can configure the **winget** command line experience by modifying the **settings.json** file. For more information, see [https://aka.ms/winget-settings](https://aka.ms/winget-settings). Note that the settings are still in an experimental state and not yet finalized for the preview version of the tool.

## Open source details

The **winget** tool is open source software available on GitHub in the repo [https://github.com/microsoft/winget-cli/](https://github.com/microsoft/winget-cli/). The source for building the client is located in the [src folder](https://github.com/microsoft/winget-cli/tree/master/src).

The source for **winget** is contained in a Visual Studio 2019 C++ solution. To build the solution correctly, install the latest [Visual Studio with the C++ workload](https://visualstudio.microsoft.com/downloads/).

We encourage you to contribute to the **winget** source on GitHub. You must first agree to and sign the Microsoft CLA.
