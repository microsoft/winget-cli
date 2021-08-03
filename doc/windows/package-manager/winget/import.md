---
title: import Command
description: imports the list of installed applications.
ms.date: 05/02/2021
ms.topic: overview
ms.localizationpriority: medium
---

# import command (winget)

[!INCLUDE [preview-note](../../includes/package-manager-preview.md)]

The **import** command of the [winget](index.md) tool imports a JSON file of apps to install.  The **import** command combined with the [**export**](export.md) command allows you to batch install applications on your PC.

The **import** command is often used to share your developer environment or build up your PC image with your favorite apps.

## Usage

`winget import [-i] <import-file> [<options>]`

![import](images/import.png)

## Arguments

The following arguments are available.
| Argument    | Description |
|-------------|-------------|  
| **-i,--import-file** | JSON file describing the packages to install

## Options

The options allow you to customize the import experience to meet your needs.

| Option      | Description |
|-------------|-------------|  
| **--ignore-unavailable**  |  Suppresses errors if the app requested is unavailable  |
| **--ignore-versions** |  Ignores versions specified in the JSON file and installs the latest available version |

## JSON Schema
The driving force behind the **import** command is the JSON file.  You can find the schema for the JSON file [here](https://aka.ms/winget-packages.schema.1.0.json).

The JSON file includes the following hierarchy:
| Entry      | Description |
|-------------|-------------|  
| **Sources**  |  The sources application manifests come from.  |
| **Packages**  |  The collection of packages to install.  |
| **Id**  |  The Windows Package Manager package identifier used to specify the package.  |
| **Version**  |  [optional] The specific version of the package to install.  |

## Importing files

When the Windows Package Manager imports the JSON file, it attempts to install the specified applications in a serial fashion.  If the application is not available or the application is already installed, it will notify the user of that case.

![import](images/import-command.png)

You will notice in the example above, the Microsoft.WindowsTerminal was already installed. Therefore the import command skipped passed the installation.
