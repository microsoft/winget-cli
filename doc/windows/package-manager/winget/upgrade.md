---
title: upgrade Command
description: upgrades the specified application.
ms.date: 05/05/2021
ms.topic: overview
ms.localizationpriority: medium
---

# upgrade command (winget)

The **upgrade** command of the [winget](index.md) tool upgrades the specified application. Optionally, you may not specify an application, this will list all available upgrades instead.

The **upgrade** command requires that you specify the exact string to upgrade. If there is any ambiguity, you will be prompted to further filter the **upgrade** command to  an exact application.

## Usage

`winget upgrade [[-q] <query>] [<options>]`

![upgrade command](images/upgrade.png)

## Options

The options allow you to customize the upgrade experience to meet your needs.

| Option | Description |
|-------------|-------------|  
| **-m, --manifest**                   | The path to the manifest of the package                                                                                    |
| **--id**                             | Filter results by ID                                                                                                       |
| **--name**                           | Filter results by name                                                                                                     |
| **--moniker**                        | Filter results by moniker                                                                                                  |
| **-v, --version**                    | Use the specified version; the default is the latest version                                                               |
| **-s, --source**                     | Find the package using the specified source                                                                                |
| **-e, --exact**                      | Find the package using an exact match                                                                                      |
| **-i, --interactive**                | Request interactive installation; user input may be needed                                                                 |
| **-h, --silent**                    | Request silent installation                                                                                                |
| **--purge**                          | Deletes all files and directories in the package directory (portable)                                                      |
| **-o, --log**                        | Log location (if supported)                                                                                                |
| **--custom**                         | Arguments to be passed on to the installer in addition to the defaults                                                     |
| **--override**                       | Override arguments to be passed on to the installer                                                                        |
| **-l, --location**                   | Location to install to (if supported)                                                                                      |
| **--scope**                          | Select installed package scope filter (user or machine)                                                                     |
| **-a, --architecture**                | Select the architecture to install                                                                                         |
| **--locale**                         | Locale to use (BCP47 format)                                                                                               |
| **--ignore-security-hash**            | Ignore the installer hash check failure                                                                                    |
| **--ignore-local-archive-malware-scan** | Ignore the malware scan performed as part of installing an archive-type package from a local manifest                    |
| **--accept-package-agreements**       | Accept all license agreements for packages                                                                                  |
| **--accept-source-agreements**        | Accept all source agreements during source operations                                                                       |
| **--header**                          | Optional Windows-Package-Manager REST source HTTP header                                                                   |
| **-r, --recurse, --all**             | Upgrade all installed packages to the latest version if available                                                          |
| **-u, --unknown, --include-unknown** | Upgrade packages even if their current version cannot be determined                                                         |
| **--pinned,--include-pinned**        | Upgrade packages even if they have a non-blocking pin                                                                      |
| **--uninstall-previous**             | Uninstall the previous version of the package during the upgrade                                                            |
| **--force**                          | Run the command directly and continue with non-security-related issues                                                     |
| **-?, --help**                       | Show help about the selected command                                                                                        |
| **--wait**                           | Prompts the user to press any key before exiting                                                                            |
| **--logs,--open-logs**               | Open the default logs location                                                                                             |
| **--verbose,--verbose-logs**         | Enable verbose logging for winget                                                                                          |
| **--disable-interactivity**          | Disable interactive prompts                                                                                                |
| **--include-unknown** | Attempt to upgrade a package even if the package's current version is unknown. | 
### Example queries

The following example upgrades a specific version of an application.

```CMD
winget upgrade powertoys --version 0.15.2
```

The following example upgrades an application from its ID.

```CMD
winget upgrade --id Microsoft.PowerToys
```

The following example shows upgrading all apps

```CMD
winget upgrade --all
```

## Using **list** and **upgrade**

It is common to use the [**list**](list.md) command to identify apps in need of an update, and then to use **upgrade** to install the latest.

In the example below you will see [**list**](list.md) identifies that an update is available for **JetBrains.Toolbox**, and then the user uses **upgrade** to update the application.

![upgrade command usage](images/upgrade.gif)

## **upgrade** --all

**upgrade --all** will identify all the applications with upgrades available. When you run **winget upgrade --all** the Windows Package Manager will look for all applications that have updates available and attempt to install the upgrade.

## Related topics

* [Use the winget tool to install and manage applications](index.md)
