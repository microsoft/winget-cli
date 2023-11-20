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

`winget upgrade [[-q] <query>...] [<options>]`

The following command aliases are available: \
`update`

![upgrade command](images/upgrade.png)

## Options

The options allow you to customize the upgrade experience to meet your needs.

| Option                            | Description                                                                                                                  |
|-----------------------------------|------------------------------------------------------------------------------------------------------------------------------|
| **-m, --manifest**                | Must be followed by the path to the manifest (YAML) file. You can use the manifest to run the upgrade experience from a [local YAML file](#local-upgrade). |
| **--id**                          | Limits the upgrade to the ID of the application.                                                                             |
| **--name**                        | Limits the search to the name of the application.                                                                           |
| **--moniker**                     | Limits the search to the moniker listed for the application.                                                                |
| **-v, --version**                 | Enables you to specify an exact version to upgrade. If not specified, the latest will upgrade the highest versioned application. |
| **-s, --source**                  | Restricts the search to the source name provided. Must be followed by the source name.                                      |
| **-e, --exact**                   | Uses the exact string in the query, including checking for case-sensitivity. It will not use the default behavior of a substring. |
| **-i, --interactive**             | Runs the installer in interactive mode. The default experience shows installer progress.                                      |
| **-h, --silent**                  | Runs the installer in silent mode. This suppresses all UI. The default experience shows installer progress.                 |
| **-o, --log**                     | Directs the logging to a log file. You must provide a path to a file that you have the write rights to.                      |
| **--override**                    | A string that will be passed directly to the installer.                                                                     |
| **-l, --location**                | Location to upgrade to (if supported).                                                                                     |
| **--force**                       | When a hash mismatch is discovered will ignore the error and attempt to install the package.                                |
| **--all**                         | Updates all available packages to the latest application.                                                                  |
| **--include-unknown**            | Attempt to upgrade a package even if the package's current version is unknown.                                              |
| **--purge**                       | Deletes all files and directories in the package directory (portable).                                                      |
| **--custom**                      | Arguments to be passed on to the installer in addition to the defaults.                                                     |
| **--scope**                       | Select installed package scope filter (user or machine).                                                                    |
| **-a, --architecture**            | Select the architecture to install.                                                                                         |
| **--locale**                      | Locale to use (BCP47 format).                                                                                               |
| **--ignore-security-hash**        | Ignore the installer hash check failure.                                                                                    |
| **--ignore-local-archive-malware-scan** | Ignore the malware scan performed as part of installing an archive-type package from a local manifest.                  |
| **--accept-package-agreements**   | Accept all license agreements for packages.                                                                                 |
| **--accept-source-agreements**    | Accept all source agreements during source operations.                                                                      |
| **--header**                      | Optional Windows-Package-Manager REST source HTTP header.                                                                   |
| **-r, --recurse, --all**         | Upgrade all installed packages to the latest version if available.                                                           |
| **--pinned,--include-pinned**    | Upgrade packages even if they have a non-blocking pin.                                                                      |
| **--uninstall-previous**         | Uninstall the previous version of the package during the upgrade.                                                            |
| **--wait**                        | Prompts the user to press any key before exiting.                                                                           |
| **--logs,--open-logs**           | Open the default logs location.                                                                                             |
| **--verbose,--verbose-logs**     | Enable verbose logging for winget.                                                                                          |
| **--disable-interactivity**      | Disable interactive prompts.                                                                                                |
| **--installer-type**             |        Select the installer type
| **--skip-dependencies**          |        Skips processing package dependencies and Windows features                                                           |

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
